# Implicit vs Explicit Kinematic Wave Solvers: GPU Implementation Analysis

This document analyzes the trade-offs between implicit and explicit kinematic wave solvers and their implications for GPU parallelization in EF5.

## 1. Current Implementation Analysis

### 1.1 What EF5 Currently Uses: Semi-Implicit Scheme

The current EF5 kinematic wave router uses a **local implicit, global explicit** scheme:

```
Current Discretization (Backward-Difference in Space, Mixed in Time):

(Δt/Δx) * Q[i,n+1] + α * Q[i,n+1]^β = (Δt/Δx) * Q[i-1,n+1] + α * Q[i,n]^β + Δt * q_lateral

Where:
- Q[i,n+1] = discharge at cell i, new timestep (UNKNOWN)
- Q[i-1,n+1] = discharge at upstream cell, new timestep (KNOWN - already computed)
- Q[i,n] = discharge at cell i, previous timestep (KNOWN)
- q_lateral = lateral inflow from water balance
```

**Key Characteristics:**
- **Local implicit:** Each cell solves a nonlinear equation for its own Q using Newton-Raphson
- **Global explicit:** Upstream cells must be computed before downstream cells
- **Upstream dependency:** Requires wavefront processing order

### 1.2 Numerical Scheme Classification

```
┌─────────────────────────────────────────────────────────────────┐
│                    SOLVER CLASSIFICATION                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  EXPLICIT          SEMI-IMPLICIT           FULLY IMPLICIT        │
│  (Forward Euler)   (Current EF5)          (Proposed)            │
│                                                                  │
│  Q[n+1] = f(Q[n])  Q[i,n+1] = f(Q[i-1,n+1], Q[i,n])            │
│                                            AQ[n+1] = b          │
│                                                                  │
│  ✗ CFL limited     ✓ Larger Δt possible   ✓ Unconditionally     │
│  ✗ Small timesteps ✓ Sequential sweep         stable            │
│  ✓ No iteration    ✓ Local Newton-Raphson ✗ Global matrix solve │
│  ✓ Trivially       ~ Wavefront parallel   ✓ Fully parallel      │
│    parallel                                  (different algo)   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Fully Implicit Kinematic Wave Formulation

### 2.1 The Coupled System

A fully implicit scheme treats ALL cells simultaneously:

```
For each cell i, the kinematic wave equation becomes:

(Δt/Δx) * Q[i]^(n+1) - (Δt/Δx) * Q[upstream(i)]^(n+1) + α * Q[i]^(n+1)^β = RHS[i]

This creates a coupled system of N nonlinear equations:
F(Q) = 0

Where Q = [Q₁, Q₂, ..., Qₙ]ᵀ is the vector of all cell discharges.
```

### 2.2 Linearization (Newton's Method)

At each Newton iteration k:

```
J(Q^k) * δQ = -F(Q^k)
Q^(k+1) = Q^k + δQ

Where J is the Jacobian matrix:
J[i,j] = ∂F[i]/∂Q[j]

For kinematic wave on a tree network:
- J[i,i] = (Δt/Δx) + α*β*Q[i]^(β-1)     (diagonal)
- J[i,upstream(i)] = -(Δt/Δx)            (off-diagonal, upstream connection)
- J[i,j] = 0 for non-connected cells
```

### 2.3 Matrix Structure

The Jacobian matrix has a **sparse tree structure**:

```
For a simple 6-cell network:

    [1] → [2] → [4]
              ↘
    [3] ────→ [5] → [6]

Jacobian structure (• = non-zero):
       1   2   3   4   5   6
    ┌                         ┐
  1 │ •                       │
  2 │ •   •                   │
  3 │         •               │
  4 │     •       •           │
  5 │     •   •       •       │
  6 │                 •   •   │
    └                         ┘

Properties:
- Lower triangular (if ordered upstream → downstream)
- Sparse: only 2N-1 non-zeros for N cells
- Tree structure: no cycles
```

---

## 3. GPU Implementation Strategies

### 3.1 Strategy Comparison

| Aspect | Semi-Implicit (Current) | Fully Implicit |
|--------|------------------------|----------------|
| **Parallelism** | Wavefront (level-by-level) | Matrix solve (all cells) |
| **Sync points** | O(D) barriers (D = depth) | O(k) iterations (k = Newton steps) |
| **Memory** | O(N) states | O(N) + sparse matrix storage |
| **Timestep** | Limited by accuracy | Unconditionally stable |
| **GPU algorithm** | Custom wavefront kernel | Sparse linear solver |
| **Libraries** | Custom CUDA | cuSPARSE, AmgX, PETSc |

### 3.2 Fully Implicit GPU Implementation

#### Option A: Direct Sparse Triangular Solve

Since the Jacobian is **lower triangular** (for properly ordered nodes), we can use:

```cpp
// Using cuSPARSE for triangular solve
#include <cusparse.h>

void implicitKinematicStep(
    cusparseHandle_t handle,
    cusparseSpMatDescr_t jacobian,  // CSR format
    cusparseDnVecDescr_t rhs,
    cusparseDnVecDescr_t solution
) {
    // Sparse triangular solve: J * δQ = -F
    cusparseSpSV_solve(handle, CUSPARSE_OPERATION_NON_TRANSPOSE,
                       &alpha, jacobian, rhs, solution, ...);
}
```

**Complexity:** O(nnz/P + D) where nnz = non-zeros, P = processors, D = depth

**Challenge:** Triangular solves have limited parallelism (same wavefront issue!)

#### Option B: Algebraic Multigrid (AMG)

For more general cases or when reordering isn't optimal:

```cpp
// Using NVIDIA AmgX
#include <amgx_c.h>

AMGX_config_handle cfg;
AMGX_config_create(&cfg, "config_version=2, "
    "solver(main)=FGMRES, "
    "main:preconditioner(amg)=AMG, "
    "main:max_iters=100, "
    "main:tolerance=1e-6");

AMGX_solver_handle solver;
AMGX_solver_create(&solver, resources, mode, cfg);

// Setup matrix (once per simulation)
AMGX_matrix_upload_all(matrix, n, nnz, 1, 1,
                       row_ptrs, col_indices, values, NULL);

// Solve each timestep
AMGX_solver_setup(solver, matrix);
AMGX_solver_solve(solver, rhs, solution);
```

**Complexity:** O(N/P) per V-cycle, O(log N) cycles typical

**Advantage:** Better parallelism than triangular solve

#### Option C: Iterative Jacobi/Gauss-Seidel on GPU

```cuda
__global__ void jacobiIteration(
    float* Q_new,
    float* Q_old,
    float* rhs,
    int* upstream,
    float* alpha,
    float* beta,
    float dt_dx,
    int N
) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    float diag = dt_dx + alpha[i] * beta[i] * powf(Q_old[i], beta[i] - 1.0f);
    float upstream_contrib = (upstream[i] >= 0) ? dt_dx * Q_old[upstream[i]] : 0.0f;

    Q_new[i] = (rhs[i] + upstream_contrib) / diag;
}

// Host code: iterate until convergence
for (int iter = 0; iter < MAX_ITER; iter++) {
    jacobiIteration<<<blocks, threads>>>(Q_new, Q_old, rhs, ...);
    swap(Q_new, Q_old);

    if (checkConvergence(Q_new, Q_old) < tolerance) break;
}
```

**Issue:** Jacobi converges slowly; Gauss-Seidel has wavefront dependency

---

## 4. Trade-offs Analysis

### 4.1 Stability and Timestep

```
COURANT-FRIEDRICHS-LEWY (CFL) CONDITION:

Explicit:     Δt ≤ Δx / c_max        (c = wave celerity)
Semi-Implicit: Δt ≤ K * Δx / c_max   (K ≈ 2-5, less restrictive)
Fully Implicit: Δt unlimited          (unconditionally stable)

Example for 1km cells, c_max = 2 m/s:
- Explicit:      Δt ≤ 500 seconds
- Semi-Implicit: Δt ≤ 1000-2500 seconds
- Fully Implicit: Δt = any (limited by accuracy, not stability)
```

### 4.2 Accuracy vs Efficiency Trade-off

```
                    ACCURACY
                       ↑
                       │
        Implicit       │     Explicit
        Small Δt       │     Small Δt
        (overkill)     │     (optimal)
                       │
        ─ ─ ─ ─ ─ ─ ─ ─┼─ ─ ─ ─ ─ ─ ─ → EFFICIENCY
                       │
        Implicit       │     Explicit
        Large Δt       │     Large Δt
        (good)         │     (UNSTABLE!)
                       │
                       ↓

Sweet spot: Implicit with moderate Δt (accuracy-limited, not stability-limited)
```

### 4.3 Computational Cost Breakdown

| Component | Semi-Implicit | Fully Implicit |
|-----------|--------------|----------------|
| **Matrix assembly** | None | O(N) per outer iteration |
| **Solve per timestep** | O(N) wavefront | O(N·k) where k = Newton iters |
| **Newton iterations** | 10 local per cell | 3-5 global |
| **Inner iterations** | None | 10-50 (linear solver) |
| **Memory bandwidth** | Low | High (matrix ops) |
| **Total FLOPS** | ~100N per timestep | ~500-2000N per timestep |

### 4.4 When Each Approach Wins

**Semi-Implicit (Current) is Better When:**
- Timestep is already small (e.g., 5-15 minute intervals for flash flood)
- Network is deep (many levels → limited implicit parallelism)
- Memory is constrained (no matrix storage needed)
- Real-time forecasting (predictable, low latency)

**Fully Implicit is Better When:**
- Very large timesteps desired (e.g., daily routing)
- Stiff systems (steep slopes, high velocities)
- Backwater effects included (diffusive wave, not just kinematic)
- Using existing GPU sparse solver infrastructure

---

## 5. Hybrid Approach: Best of Both Worlds

### 5.1 Proposed Hybrid Strategy

```
┌─────────────────────────────────────────────────────────────────┐
│                    HYBRID APPROACH                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. HILLSLOPE CELLS: Semi-Implicit (Wavefront)                  │
│     - Many cells, shallow network                                │
│     - Fast local Newton-Raphson                                  │
│     - Excellent GPU parallelism                                  │
│                                                                  │
│  2. CHANNEL CELLS: Fully Implicit (Sparse Solve)                │
│     - Fewer cells, deeper network                                │
│     - May need larger timesteps                                  │
│     - Better stability for steep channels                        │
│                                                                  │
│  3. COUPLING: Channel receives hillslope contribution           │
│     - Hillslope → Channel at each timestep                       │
│     - One-way coupling (no backwater)                            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Implementation Sketch

```cpp
void hybridRoute(float dt) {
    // Phase 1: Hillslope routing (semi-implicit, wavefront parallel)
    for (int level = 0; level <= maxHillslopeLevel; level++) {
        hillslopeKernel<<<...>>>(hillslopeCells[level], ...);
        cudaDeviceSynchronize();
    }

    // Phase 2: Accumulate hillslope contributions to channels
    accumulateToChannels<<<...>>>(hillslopeOutflow, channelLateralInflow);

    // Phase 3: Channel routing (fully implicit)
    assembleChannelJacobian<<<...>>>(channelJacobian, channelRHS);

    for (int newton = 0; newton < MAX_NEWTON; newton++) {
        // Sparse triangular solve for channel network
        cusparseSpSV_solve(..., channelJacobian, channelRHS, channelQ);

        if (newtonConverged()) break;

        updateJacobian<<<...>>>(channelQ, channelJacobian);
    }
}
```

---

## 6. Recommended GPU Libraries for Implicit Solvers

### 6.1 Library Comparison

| Library | Type | Strengths | GPU Support |
|---------|------|-----------|-------------|
| **cuSPARSE** | Direct | Fast triangular solve | NVIDIA native |
| **AmgX** | AMG | Excellent for large systems | NVIDIA native |
| **PETSc** | General | Flexible, many algorithms | CUDA, HIP, Kokkos |
| **Trilinos** | General | Scalable, MPI+GPU | Kokkos backend |
| **hypre** | AMG | Proven at scale | CUDA, HIP |
| **MAGMA** | Dense/Sparse | High performance | CUDA, HIP |

### 6.2 Recommended Stack

```
For EF5 Implicit Solver:

Primary:   cuSPARSE (triangular solve for tree networks)
           └── Fast, native CUDA, exploits tree structure

Fallback:  AmgX (when triangular solve is slow)
           └── Better parallelism for complex networks

Future:    PETSc with CUDA (if diffusive wave needed)
           └── Supports more general equation systems
```

---

## 7. Diffusive Wave Extension

### 7.1 Why Consider Diffusive Wave?

The kinematic wave assumes `∂h/∂x = S₀` (bed slope). This fails for:
- Backwater effects (downstream control)
- Mild slopes
- Tidal boundaries
- Dam breaks

The **diffusive wave** adds a pressure gradient term:

```
Kinematic:  S_f = S_0                    (friction slope = bed slope)
Diffusive:  S_f = S_0 - ∂h/∂x           (includes water surface slope)

Diffusive wave equation:
∂Q/∂t + c·∂Q/∂x = D·∂²Q/∂x²

Where D = Q / (2·B·S_f) is diffusion coefficient
```

### 7.2 GPU Implementation for Diffusive Wave

The diffusive wave creates a **tridiagonal** or **pentadiagonal** system:

```
For 1D channel:
[b₁ c₁  0  0  0 ] [Q₁]   [r₁]
[a₂ b₂ c₂  0  0 ] [Q₂]   [r₂]
[ 0 a₃ b₃ c₃  0 ] [Q₃] = [r₃]
[ 0  0 a₄ b₄ c₄] [Q₄]   [r₄]
[ 0  0  0 a₅ b₅] [Q₅]   [r₅]

GPU algorithm: Cyclic Reduction or Parallel Cyclic Reduction (PCR)
Complexity: O(N/P + log N)
```

**CUDA Library:** cuSPARSE `gtsv2` (tridiagonal solver)

```cpp
cusparseSgtsv2_bufferSizeExt(handle, n, 1, dl, d, du, B, ldb, &bufferSize);
cusparseSgtsv2(handle, n, 1, dl, d, du, B, ldb, pBuffer);
```

---

## 8. Performance Projections

### 8.1 Estimated GPU Performance

| Method | 100K cells | 1M cells | 10M cells |
|--------|-----------|----------|-----------|
| **Semi-Implicit (wavefront)** | 2 ms | 15 ms | 120 ms |
| **Fully Implicit (triangular)** | 3 ms | 25 ms | 200 ms |
| **Fully Implicit (AMG)** | 5 ms | 20 ms | 80 ms |
| **Hybrid (hillslope + channel)** | 2.5 ms | 12 ms | 70 ms |

*Estimates for RTX 3080, single timestep*

### 8.2 Timestep Trade-off

```
Total computation time = (Simulation Duration / Δt) × (Time per step)

Example: 24-hour simulation, 1M cells

Semi-Implicit, Δt=5min:   (1440/5) × 15ms  = 4.3 seconds
Fully Implicit, Δt=30min: (1440/30) × 80ms = 3.8 seconds
Fully Implicit, Δt=60min: (1440/60) × 100ms = 2.4 seconds

Conclusion: Implicit wins only if timestep can be ~4-6x larger
```

---

## 9. Recommendations

### 9.1 Short-term (Recommended First Step)

**Keep semi-implicit, implement wavefront GPU parallelization:**
- Lower implementation complexity
- Good for flash flood forecasting (small timesteps anyway)
- Predictable performance
- No external library dependencies

### 9.2 Medium-term (If Larger Timesteps Needed)

**Add fully implicit option for channel routing:**
- Use cuSPARSE triangular solve
- Keep semi-implicit for hillslopes
- Hybrid approach balances parallelism and timestep flexibility

### 9.3 Long-term (For Advanced Applications)

**Implement diffusive wave with AMG solver:**
- Required for backwater effects
- Use AmgX or PETSc
- Enables new applications (dam break, tidal, reservoirs)

---

## 10. Implementation Checklist

### Phase 1: Semi-Implicit GPU (Current Approach)
- [ ] Implement topological level assignment
- [ ] Create SoA data structures
- [ ] Write wavefront CUDA kernel
- [ ] Benchmark and optimize

### Phase 2: Fully Implicit Option
- [ ] Implement Jacobian assembly kernel
- [ ] Integrate cuSPARSE triangular solve
- [ ] Add Newton iteration loop
- [ ] Compare performance vs semi-implicit

### Phase 3: Diffusive Wave Extension
- [ ] Modify governing equations
- [ ] Implement tridiagonal solver for channels
- [ ] Add boundary condition handling
- [ ] Validate against analytical solutions

---

## References

1. Cunge, J.A., Holly, F.M., & Verwey, A. (1980). *Practical Aspects of Computational River Hydraulics*. Pitman.

2. Sanders, B.F. (2008). "Integration of a shallow water model with a local time step." *Journal of Hydraulic Research*, 46(4), 466-475.

3. Caviedes-Voullième, D., et al. (2020). "GPU-accelerated shallow water flow solvers." *Journal of Hydroinformatics*, 22(4), 784-802.

4. Bell, N., & Garland, M. (2008). "Efficient sparse matrix-vector multiplication on CUDA." NVIDIA Technical Report NVR-2008-004.

5. NVIDIA cuSPARSE Documentation (2024). https://docs.nvidia.com/cuda/cusparse/

6. NVIDIA AmgX Documentation (2024). https://developer.nvidia.com/amgx
