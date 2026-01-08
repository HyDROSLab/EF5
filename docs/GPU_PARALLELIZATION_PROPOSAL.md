# GPU Parallelization Proposal for EF5

This document proposes strategies for parallelizing the water balance and routing models in EF5 for GPU execution, focusing on the Kinematic Wave routing algorithm.

## Executive Summary

EF5's computational pipeline consists of two main stages with different parallelization characteristics:

| Stage | Parallelism Type | GPU Suitability | Expected Speedup |
|-------|-----------------|-----------------|------------------|
| Water Balance | Embarrassingly Parallel | Excellent | 10-100x |
| Routing | Wavefront Parallel | Good | 5-20x |

The water balance can achieve near-linear speedup with GPU cores. Routing requires more sophisticated algorithms but can still achieve significant speedup using **wavefront parallelism** or **parallel graph algorithms**.

---

## 1. Current Algorithm Analysis

### 1.1 Water Balance (Fully Parallel)

The water balance computation at each grid cell is **completely independent**:

```cpp
// Current implementation - sequential but independent
for (size_t i = 0; i < numNodes; i++) {
    waterBalance->WaterBalance(node[i], precip[i], pet[i]);
}
```

Each cell only reads its local precipitation and PET, updates its local state, and outputs fastFlow/slowFlow. **No inter-cell dependencies exist**.

### 1.2 Kinematic Wave Routing (Data Dependencies)

The routing has **upstream-to-downstream dependencies**:

```cpp
// Current implementation - must process upstream first
for (long i = numNodes - 1; i >= 0; i--) {  // Reverse topological order
    RouteInt(node[i], fastFlow[i], slowFlow[i]);
    // Writes to downstream cell:
    // downstream.incomingWaterChannel += thisCell.outflow
}
```

**Key Data Dependencies:**
1. `incomingWaterOverland` - accumulated from upstream hillslope cells
2. `incomingWaterChannel` - accumulated from upstream channel cells
3. `incomingWater[KW_LAYER_INTERFLOW]` - accumulated interflow from upstream

A cell cannot compute its outflow until ALL upstream cells have contributed their inflow.

### 1.3 Dependency Graph Structure

The flow network forms a **Directed Acyclic Graph (DAG)** with tree structure:
- Multiple headwaters (sources, no incoming edges)
- Single outlet (sink, no outgoing edge)
- Each cell has exactly one downstream neighbor (D8 flow direction)
- Cells can have 0-8 upstream neighbors

```
Level 0:  [H1] [H2] [H3] [H4] [H5] [H6]    <- Headwaters (parallel)
             \   |   /       \   |
Level 1:      [C1]            [C2]          <- Can process in parallel
                 \            /
Level 2:           [C3]----[C4]             <- Can process in parallel
                       \  /
Level 3:               [O]                   <- Outlet
```

---

## 2. Proposed Parallelization Strategy

### 2.1 Wavefront Parallelism (Recommended)

**Concept:** Group cells into "levels" based on their topological distance from headwaters. All cells at the same level can be processed in parallel.

**Algorithm: Level Assignment (Preprocessing)**

```cpp
// Run once during initialization
void AssignTopologicalLevels(vector<GridNode>& nodes, vector<int>& levels) {
    int numNodes = nodes.size();
    vector<int> inDegree(numNodes, 0);

    // Count upstream neighbors for each cell
    for (int i = 0; i < numNodes; i++) {
        if (nodes[i].downStreamNode != INVALID_DOWNSTREAM_NODE) {
            inDegree[nodes[i].downStreamNode]++;
        }
    }

    // Initialize headwaters (no upstream neighbors) at level 0
    queue<int> frontier;
    for (int i = 0; i < numNodes; i++) {
        if (inDegree[i] == 0) {
            levels[i] = 0;
            frontier.push(i);
        }
    }

    // BFS to assign levels
    int maxLevel = 0;
    while (!frontier.empty()) {
        int current = frontier.front();
        frontier.pop();

        int downstream = nodes[current].downStreamNode;
        if (downstream != INVALID_DOWNSTREAM_NODE) {
            levels[downstream] = max(levels[downstream], levels[current] + 1);
            maxLevel = max(maxLevel, levels[downstream]);

            inDegree[downstream]--;
            if (inDegree[downstream] == 0) {
                frontier.push(downstream);
            }
        }
    }
}
```

**GPU Execution Strategy:**

```cpp
// Preprocessing: Group cells by level
vector<vector<int>> levelGroups(maxLevel + 1);
for (int i = 0; i < numNodes; i++) {
    levelGroups[levels[i]].push_back(i);
}

// Runtime: Process level by level
for (int level = 0; level <= maxLevel; level++) {
    // Launch GPU kernel for all cells at this level (parallel)
    int numCellsAtLevel = levelGroups[level].size();

    routeKernel<<<numBlocks, threadsPerBlock>>>(
        d_nodes, d_levelGroups[level], numCellsAtLevel,
        d_fastFlow, d_slowFlow, d_discharge
    );

    cudaDeviceSynchronize();  // Wait for level to complete before next
}
```

**GPU Kernel (CUDA-style pseudocode):**

```cuda
__global__ void routeKernel(
    KWGridNode* nodes,
    int* cellIndices,
    int numCells,
    float* fastFlow,
    float* slowFlow,
    float stepSeconds
) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= numCells) return;

    int cellIdx = cellIndices[tid];
    KWGridNode* cNode = &nodes[cellIdx];

    // Solve kinematic wave equation (Newton-Raphson)
    float newQ = solveKinematicWave(cNode, fastFlow[cellIdx], slowFlow[cellIdx]);

    // Update state
    cNode->states[STATE_KW_PQ] = newQ;

    // Atomic add to downstream cell's incoming water
    int downstream = nodes[cellIdx].downStreamNode;
    if (downstream != INVALID_DOWNSTREAM_NODE) {
        atomicAdd(&nodes[downstream].incomingWaterChannel, newQ);
    }
}
```

### 2.2 Expected Parallelism Analysis

For a typical watershed:

| Metric | Typical Value | Impact |
|--------|--------------|--------|
| Total cells | 100,000 - 10,000,000 | More cells = better GPU utilization |
| Number of levels | 100 - 5,000 | More levels = more synchronization overhead |
| Cells per level (avg) | 1,000 - 100,000 | Higher = better parallelism |
| Max cells at one level | 10,000 - 500,000 | Determines peak parallelism |

**Parallelism efficiency:**
- Headwater regions: Excellent (many independent cells)
- Channel network: Lower (fewer cells, linear dependency chains)
- Overall: 50-90% parallel efficiency typical

---

## 3. Data Structure Modifications for GPU

### 3.1 Structure of Arrays (SoA) Layout

Current **Array of Structures (AoS)** layout is cache-inefficient for GPU:

```cpp
// Current: AoS (poor GPU memory coalescing)
struct KWGridNode {
    float params[7];
    double states[3];
    double incomingWater[2];
    // ...
};
vector<KWGridNode> kwNodes;
```

**Proposed: Structure of Arrays (SoA)**

```cpp
// Better for GPU: SoA
struct KWGridNodesSoA {
    // Parameters (read-only after init)
    float* alpha;         // [numNodes]
    float* alpha0;        // [numNodes]
    float* beta;          // [numNodes]
    float* leaki;         // [numNodes]

    // States (read-write)
    float* state_PQ;      // [numNodes]
    float* state_PO;      // [numNodes]
    float* state_IR;      // [numNodes]

    // Incoming water (atomic updates)
    float* incomingChannel;   // [numNodes]
    float* incomingOverland;  // [numNodes]
    float* incomingInterflow; // [numNodes]

    // Topology (read-only)
    int* downstreamNode;  // [numNodes]
    int* level;           // [numNodes]
    bool* isChannel;      // [numNodes]
};
```

### 3.2 Memory Access Pattern

```
GPU Memory Layout:
┌─────────────────────────────────────────────┐
│ Parameters (read-only, cached in L2)        │
│ ├── alpha[0..N-1]                           │
│ ├── beta[0..N-1]                            │
│ └── ...                                     │
├─────────────────────────────────────────────┤
│ States (read-write, main working set)       │
│ ├── state_PQ[0..N-1]                        │
│ ├── state_PO[0..N-1]                        │
│ └── state_IR[0..N-1]                        │
├─────────────────────────────────────────────┤
│ Incoming (atomic updates, high contention)  │
│ ├── incomingChannel[0..N-1]                 │
│ └── incomingOverland[0..N-1]                │
└─────────────────────────────────────────────┘
```

---

## 4. Optimization Strategies

### 4.1 Reducing Synchronization Overhead

**Problem:** Processing level-by-level requires `cudaDeviceSynchronize()` between levels.

**Solution 1: Level Coalescing**
Merge consecutive levels if they have few cells:

```cpp
// If level has < MIN_CELLS_PER_KERNEL cells, merge with next level
// This reduces kernel launch overhead
vector<LevelBatch> batches = coalesceLevels(levelGroups, MIN_CELLS_PER_KERNEL);
```

**Solution 2: Persistent Kernels with Barriers**
Use CUDA cooperative groups for grid-wide synchronization:

```cuda
#include <cooperative_groups.h>

__global__ void persistentRouteKernel(/* ... */) {
    namespace cg = cooperative_groups;
    cg::grid_group grid = cg::this_grid();

    for (int level = 0; level <= maxLevel; level++) {
        // Process cells at this level
        if (myLevel == level) {
            processCell(myCellIdx);
        }

        // Grid-wide barrier (all threads sync)
        grid.sync();
    }
}
```

### 4.2 Handling Atomic Contention

**Problem:** Multiple upstream cells writing to same downstream cell causes contention.

**Analysis:** Most cells have 1-3 upstream neighbors, so contention is moderate.

**Solution 1: Hardware Atomics (Simple)**
Modern GPUs have fast atomic operations:
```cuda
atomicAdd(&downstream.incomingWater, myOutflow);
```

**Solution 2: Thread-Local Accumulation**
For high-contention scenarios:
```cuda
// Each warp accumulates locally first
__shared__ float localAccum[32][MAX_DOWNSTREAM];
// Then one thread per downstream does final atomic
```

**Solution 3: Separate Read/Write Buffers (Double Buffering)**
```cpp
// Level N writes to buffer A, Level N+1 reads from buffer A and writes to buffer B
float* incomingRead;   // Previous level's output
float* incomingWrite;  // Current level's output
swap(incomingRead, incomingWrite);  // After each level
```

### 4.3 Newton-Raphson Convergence

The iterative solver can have variable iteration counts:

```cuda
// Current: Fixed 10 iterations (may be wasteful or insufficient)
for (int itr = 0; itr < 10; itr++) { ... }

// Better: Early exit with warp voting
for (int itr = 0; itr < MAX_ITER; itr++) {
    float error = computeError();
    // Check if all threads in warp have converged
    if (__all_sync(0xFFFFFFFF, error < TOLERANCE)) break;
    // Update estimate
}
```

---

## 5. Alternative Algorithms

### 5.1 Parallel Tree Contraction

For strictly tree-structured networks (no braiding), the **Euler tour technique** and **parallel tree contraction** can achieve O(log N) depth:

1. Convert flow tree to Euler tour representation
2. Use parallel prefix sum to compute accumulated flows
3. Complexity: O(N/P + log N) for P processors

**Best for:** Simple dendritic networks without complex routing physics.

### 5.2 Domain Decomposition

Split the watershed into sub-basins processed independently:

```
┌─────────────────────────────────────┐
│         Sub-basin 1 (GPU 1)         │
│    ┌───────┐     ┌───────┐          │
│    │ H1-H3 │────→│  C1   │──┐       │
│    └───────┘     └───────┘  │       │
├─────────────────────────────│───────┤
│         Sub-basin 2 (GPU 2) │       │
│    ┌───────┐     ┌───────┐  │       │
│    │ H4-H6 │────→│  C2   │──│       │
│    └───────┘     └───────┘  │       │
├─────────────────────────────▼───────┤
│         Junction (sync point)       │
│                  │                  │
│                  ▼                  │
│               [Outlet]              │
└─────────────────────────────────────┘
```

**Advantages:**
- Can use multiple GPUs
- Reduces global synchronization

**Disadvantages:**
- Requires ghost cell exchange
- Load balancing challenges

### 5.3 Algebraic Multigrid (for implicit methods)

If switching to an implicit kinematic wave solver, algebraic multigrid can be parallelized effectively:

1. Formulate as sparse linear system: **Ax = b**
2. Use GPU-accelerated AMG solver (e.g., AmgX, hypre)
3. Good for stiff systems and large timesteps

---

## 6. Implementation Roadmap

### Phase 1: Water Balance on GPU (2-4 weeks)
- Port water balance models to CUDA kernels
- Implement SoA data structures
- Expected speedup: **10-50x** for water balance alone

### Phase 2: Level-Based Routing (4-6 weeks)
- Implement topological level assignment
- Create GPU routing kernel with atomic updates
- Add synchronization between levels
- Expected speedup: **5-15x** for routing

### Phase 3: Optimization (2-4 weeks)
- Profile and optimize memory access patterns
- Implement level coalescing
- Tune kernel launch parameters
- Target: **15-30x** overall speedup

### Phase 4: Multi-GPU Support (optional, 4-6 weeks)
- Domain decomposition for multiple GPUs
- Peer-to-peer memory access for ghost cells
- Useful for continental-scale simulations

---

## 7. Performance Estimates

### 7.1 Baseline Comparison

| Configuration | Time per Timestep | Relative Speed |
|--------------|-------------------|----------------|
| CPU Single-threaded (current) | 100ms | 1x |
| CPU Multi-threaded (8 cores) | 15ms | 6.7x |
| GPU Naive (water balance only) | 5ms | 20x |
| GPU Optimized (full pipeline) | 3ms | 33x |

*Estimates for 1M cell watershed on RTX 3080*

### 7.2 Scaling Analysis

```
Speedup vs. Grid Size (estimated):

Speedup │                         ████
  50x   │                    ████ ████
        │               ████ ████ ████
  30x   │          ████ ████ ████ ████
        │     ████ ████ ████ ████ ████
  10x   │████ ████ ████ ████ ████ ████
        │████ ████ ████ ████ ████ ████
   1x   └────────────────────────────────
        10K  100K  500K   1M   5M   10M
                   Grid Cells
```

GPU parallelization becomes increasingly beneficial at larger scales.

---

## 8. Technology Recommendations

### 8.1 GPU Framework

| Framework | Pros | Cons | Recommendation |
|-----------|------|------|----------------|
| **CUDA** | Best performance, mature | NVIDIA-only | Primary target |
| **HIP** | Portable CUDA | Less mature | Secondary for AMD |
| **OpenCL** | Cross-platform | Verbose, less performant | Not recommended |
| **SYCL** | Modern C++, portable | Young ecosystem | Future option |

**Recommendation:** Start with CUDA, use HIP for AMD portability.

### 8.2 Libraries

| Library | Purpose |
|---------|---------|
| **Thrust** | GPU-accelerated STL (sorting, reduction) |
| **CUB** | Low-level GPU primitives (scan, sort) |
| **cuSPARSE** | Sparse matrix operations (if needed) |
| **RAPIDS cuGraph** | Graph algorithms (alternative approach) |

---

## 9. Code Example: GPU Wavefront Routing

```cuda
// gpu_kinematic_route.cu

#include <cuda_runtime.h>
#include <cooperative_groups.h>

namespace cg = cooperative_groups;

// SoA data structure
struct KWDataGPU {
    // Parameters
    float* alpha;
    float* alpha0;
    float* beta;
    float* leaki;
    float* horLen;

    // States
    float* state_PQ;
    float* state_PO;
    float* state_IR;

    // Flow accumulators
    float* incomingChannel;
    float* incomingOverland;

    // Topology
    int* downstream;
    int* level;
    bool* isChannel;

    // Level structure
    int* levelStart;    // Start index for each level
    int* levelSize;     // Number of cells at each level
    int* cellOrder;     // Cells sorted by level
    int maxLevel;
};

__device__ float solveKinematicWave(
    float incoming, float prevQ,
    float lateralInflow, float alpha, float beta,
    float dt, float dx
) {
    float rhs = (dt/dx) * incoming +
                alpha * powf(prevQ, beta) +
                dt * lateralInflow;

    // Initial estimate
    float q = rhs / ((dt/dx) + alpha);

    // Newton-Raphson iterations
    for (int i = 0; i < 10; i++) {
        float f = (dt/dx) * q + alpha * powf(q, beta) - rhs;
        if (fabsf(f) < 0.01f) break;

        float fp = (dt/dx) + alpha * beta * powf(q, beta - 1.0f);
        q = fmaxf(0.0f, q - f / fp);
    }

    return q;
}

__global__ void routeLevelKernel(
    KWDataGPU data,
    float* fastFlow,
    float* slowFlow,
    float dt,
    int level
) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    int levelStart = data.levelStart[level];
    int levelSize = data.levelSize[level];

    if (tid >= levelSize) return;

    int cellIdx = data.cellOrder[levelStart + tid];

    float ff = fastFlow[cellIdx] / 1000.0f;  // mm to m
    float sf = slowFlow[cellIdx] / 1000.0f;

    float alpha, beta, newQ;

    if (!data.isChannel[cellIdx]) {
        // Hillslope routing
        alpha = data.alpha0[cellIdx];
        beta = 0.6f;

        newQ = solveKinematicWave(
            data.incomingOverland[cellIdx],
            data.state_PQ[cellIdx],
            ff, alpha, beta, dt, data.horLen[cellIdx]
        );

        data.state_PQ[cellIdx] = newQ;

        // Interflow handling
        data.state_IR[cellIdx] += sf;
        float leak = data.state_IR[cellIdx] * data.leaki[cellIdx];
        data.state_IR[cellIdx] -= leak;

        // Atomic update to downstream
        int ds = data.downstream[cellIdx];
        if (ds >= 0) {
            atomicAdd(&data.incomingOverland[ds], newQ);
        }

    } else {
        // Channel routing
        alpha = data.alpha[cellIdx];
        beta = data.beta[cellIdx];

        // First hillslope contribution
        float overlandQ = solveKinematicWave(
            data.incomingOverland[cellIdx],
            data.state_PO[cellIdx],
            ff + sf, data.alpha0[cellIdx], 0.6f,
            dt, data.horLen[cellIdx]
        );
        data.state_PO[cellIdx] = overlandQ;

        // Then channel routing
        newQ = solveKinematicWave(
            data.incomingChannel[cellIdx],
            data.state_PQ[cellIdx],
            overlandQ, alpha, beta, dt, data.horLen[cellIdx]
        );
        data.state_PQ[cellIdx] = newQ;

        int ds = data.downstream[cellIdx];
        if (ds >= 0) {
            atomicAdd(&data.incomingChannel[ds], newQ);
        }
    }
}

// Host function to run routing
void gpuRoute(KWDataGPU& d_data, float* d_fastFlow, float* d_slowFlow,
              float dt, cudaStream_t stream) {

    const int BLOCK_SIZE = 256;

    for (int level = 0; level <= d_data.maxLevel; level++) {
        int numCells = d_data.levelSize[level];
        int numBlocks = (numCells + BLOCK_SIZE - 1) / BLOCK_SIZE;

        routeLevelKernel<<<numBlocks, BLOCK_SIZE, 0, stream>>>(
            d_data, d_fastFlow, d_slowFlow, dt, level
        );
    }

    cudaStreamSynchronize(stream);
}
```

---

## 10. References

1. **Wavefront Parallelism:**
   - Merrill, D., & Garland, M. (2016). "Single-pass Parallel Prefix Scan with Decoupled Look-back." *GPU Technology Conference*.

2. **Parallel Graph Algorithms:**
   - Harish, P., & Narayanan, P. J. (2007). "Accelerating Large Graph Algorithms on the GPU Using CUDA." *High Performance Computing*.

3. **Hydrological Model GPU Acceleration:**
   - Lacasta, A., et al. (2014). "GPU Implementation of the 2D Shallow Water Equations." *Journal of Hydroinformatics*.
   - Ming, Y., et al. (2020). "A GPU-Accelerated Distributed Hydrological Model." *Environmental Modelling & Software*.

4. **CUDA Optimization:**
   - NVIDIA CUDA Best Practices Guide (2024).
   - Harris, M. (2013). "Optimizing Parallel Reduction in CUDA." NVIDIA Developer.

---

## Appendix A: Complexity Analysis

| Operation | Sequential | Parallel (P processors) |
|-----------|-----------|------------------------|
| Water Balance | O(N) | O(N/P) |
| Level Assignment | O(N) | O(N/P + D) |
| Routing (per timestep) | O(N) | O(N/P + D) |
| **Total per timestep** | **O(N)** | **O(N/P + D)** |

Where:
- N = number of grid cells
- P = number of parallel processors
- D = depth of flow network (number of levels)

For typical watersheds: D ≈ O(√N), so parallel complexity ≈ O(N/P + √N)
