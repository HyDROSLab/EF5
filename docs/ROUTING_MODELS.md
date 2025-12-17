# Routing Models in EF5

This document provides detailed technical documentation for the routing models implemented in the Ensemble Framework For Flash Flood Forecasting (EF5). Routing models transport water from upstream grid cells to downstream cells, simulating the movement of water through the landscape and channel network.

## Overview

EF5 implements two routing models:

| Model | Type | Complexity | States | Parameters | Best Use Case |
|-------|------|------------|--------|------------|---------------|
| **Linear Reservoir** | Conceptual | Simple | 2 reservoirs | 8 | Fast forecasting, large-scale applications |
| **Kinematic Wave** | Physics-based | Moderate | 3 states | 7 | Detailed process studies, channel routing |

Both routing models receive water from the water balance model as two flow components:
- **Fast Flow (Overland)**: Surface runoff that moves quickly across the landscape
- **Slow Flow (Interflow)**: Subsurface lateral flow that moves more slowly

---

## Linear Reservoir Routing

**Source Files:** `src/LinearRoute.h`, `src/LinearRoute.cpp`

### Concept

The Linear Reservoir model treats each grid cell as containing two conceptual reservoirs - one for overland (fast) flow and one for interflow (slow) flow. Water enters these reservoirs from the water balance model and from upstream cells, then drains to downstream cells based on a linear outflow relationship.

### Parameters

| Parameter | Name | Description | Typical Range | Units |
|-----------|------|-------------|---------------|-------|
| `COEM` | Overland Coefficient | Manning's roughness for overland flow | 0.01 - 0.5 | - |
| `RIVER` | Channel Coefficient | Manning's roughness for channel flow | 0.01 - 0.1 | - |
| `UNDER` | Interflow Coefficient | Speed coefficient for subsurface flow | 0.001 - 0.1 | - |
| `LEAKO` | Overland Leakage | Fraction of overland reservoir released per timestep | 0.0 - 1.0 | - |
| `LEAKI` | Interflow Leakage | Fraction of interflow reservoir released per timestep | 0.0 - 1.0 | - |
| `TH` | Threshold | Flow accumulation threshold for channel cells | 1 - 1000 | cells |
| `ISO` | Initial Overland Storage | Initial water depth in overland reservoir | 0.0 - 10.0 | mm |
| `ISU` | Initial Underground Storage | Initial water depth in interflow reservoir | 0.0 - 100.0 | mm |

### Data Structure

```cpp
struct LRGridNode {
    float params[8];              // Routing parameters
    double slopeSqrt;             // Pre-calculated sqrt(slope)
    double nexTime[2];            // Travel time for each layer [OVERLAND, INTERFLOW]
    LRGridNode *routeCNode[2][2]; // Downstream cell pointers (2 layers x 2 targets)
    double routeAmount[2][2];     // Fraction of flow to each downstream cell
    double incomingWater[2];      // Accumulated incoming water from upstream
};
```

### Algorithm

#### Initialization Phase (`InitializeRouting`)

1. **Calculate Flow Velocity** using Manning's equation:
   ```
   velocity = depth^0.66 * sqrt(slope) * coefficient
   ```
   Where:
   - `depth` is assumed from hydraulic radius approximation
   - `slope` is terrain slope from DEM
   - `coefficient` is COEM (overland) or RIVER (channel) or UNDER (interflow)

2. **Determine Travel Time** across each cell:
   ```
   travelTime = cellLength / velocity
   ```

3. **Pre-compute Routing Destinations**:
   - If `travelTime < timestep`: Water reaches downstream cell(s)
   - Calculate fractional split between cells based on how far water travels
   - Store routing coefficients for runtime efficiency

#### Routing Phase (`RouteInt`)

For each timestep, the routing proceeds in order from upstream to downstream:

```
For each grid cell (upstream to downstream order):

    // OVERLAND LAYER
    1. Add incoming fast flow from upstream to overland reservoir
    2. Calculate overland leakage:
       overlandLeak = reservoir * LEAKO

    // INTERFLOW LAYER
    3. Add incoming slow flow from upstream to interflow reservoir
    4. Calculate interflow leakage:
       interflowLeak = reservoir * LEAKI

    // ROUTE TO DOWNSTREAM
    5. For each routing layer:
       - Calculate outflow based on pre-computed routing fractions
       - Split flow between primary and secondary downstream cells
       - Add to downstream cells' incoming water accumulator

    // DISCHARGE CALCULATION
    6. If gauge location:
       discharge = (overlandLeak + interflowLeak) * cellArea / timestep
```

### Routing Fraction Calculation

When water travels through a cell, it may reach one or two downstream cells depending on travel time:

```
If travelTime >= timestep:
    // All water stays in this cell for this timestep
    routeAmount[0] = 1.0  // to self
    routeAmount[1] = 0.0  // none to downstream

Else:
    remainingTime = timestep - travelTime
    // Calculate how far into downstream cell water travels
    // Split proportionally between current and downstream
    routeAmount[0] = travelTime / timestep      // fraction staying
    routeAmount[1] = remainingTime / timestep   // fraction moving downstream
```

### Strengths and Limitations

**Strengths:**
- Computationally efficient
- Simple parameterization
- Suitable for real-time forecasting
- Pre-computed routing enables fast execution

**Limitations:**
- Linear assumption may not capture nonlinear channel dynamics
- Less physically realistic for detailed channel flow studies
- Does not account for backwater effects

---

## Kinematic Wave Routing

**Source Files:** `src/KinematicRoute.h`, `src/KinematicRoute.cpp`

### Concept

The Kinematic Wave model is based on the shallow water equations with the kinematic wave approximation. This assumes that the friction slope equals the bed slope (no pressure gradient effects), which is valid for overland flow and steep channels where backwater effects are negligible.

### Governing Equations

The kinematic wave equation combines continuity and momentum:

**Continuity:**
```
dh/dt + dq/dx = i - f
```

**Kinematic Wave Assumption:**
```
q = alpha * h^beta
```

Where:
- `h` = flow depth (m)
- `q` = discharge per unit width (m^2/s)
- `i` = lateral inflow (rainfall excess)
- `f` = infiltration/losses
- `alpha` = flow coefficient
- `beta` = flow exponent (typically 0.6 for overland, variable for channels)

### Parameters

| Parameter | Name | Description | Typical Range | Units |
|-----------|------|-------------|---------------|-------|
| `UNDER` | Interflow Coefficient | Subsurface flow speed coefficient | 0.001 - 0.1 | - |
| `LEAKI` | Interflow Leakage | Fraction of interflow released per timestep | 0.0 - 1.0 | - |
| `TH` | Threshold | Flow accumulation threshold for channels | 1 - 1000 | cells |
| `ISU` | Initial Underground Storage | Initial interflow reservoir depth | 0.0 - 100.0 | mm |
| `ALPHA` | Channel Alpha | Channel flow coefficient (alpha) | 0.1 - 10.0 | - |
| `BETA` | Channel Beta | Channel flow exponent (beta) | 0.3 - 1.0 | - |
| `ALPHA0` | Hillslope Alpha | Overland flow coefficient | 0.01 - 1.0 | - |

### State Variables

| State | Name | Description |
|-------|------|-------------|
| `STATE_KW_PQ` | Previous Channel Flow | Channel discharge from previous timestep |
| `STATE_KW_PO` | Previous Overland Flow | Overland flow from previous timestep |
| `STATE_KW_IR` | Interflow Reservoir | Current interflow storage |

### Data Structure

```cpp
struct KWGridNode {
    float params[7];              // Kinematic wave parameters
    double states[3];             // State variables [PQ, PO, IR]
    double slopeSqrt;             // Pre-calculated sqrt(slope)
    double alpha[2];              // Alpha for [hillslope, channel]
    double beta[2];               // Beta for [hillslope, channel]
    double incomingWater[2];      // Incoming water [FASTFLOW, INTERFLOW]
};
```

### Algorithm

#### For Hillslope Cells (`RouteInt`)

1. **Setup Right-Hand Side (RHS)**:
   ```
   RHS = (dt/dx) * upstream_flow + precip_excess + previous_overland
   ```

2. **Solve Kinematic Wave** using Newton-Raphson iteration (10 iterations):
   ```
   For i = 1 to 10:
       f(q) = (dt/dx)*q + alpha*q^beta - RHS
       f'(q) = (dt/dx) + alpha*beta*q^(beta-1)
       q_new = q - f(q)/f'(q)
   ```

3. **Update States**:
   ```
   previous_overland = q
   Route q to downstream cell
   ```

#### For Channel Cells (`RouteInt`)

Channel cells process both hillslope contributions and channel flow:

1. **Accumulate Hillslope Contributions**:
   ```
   lateral_inflow = sum of overland flow from adjacent hillslope cells
   ```

2. **Process Interflow Reservoir**:
   ```
   interflow_leak = reservoir * LEAKI
   reservoir = reservoir * (1 - LEAKI) + incoming_interflow
   ```

3. **Solve Channel Kinematic Wave**:
   ```
   RHS = (dt/dx)*upstream_channel_flow + lateral_inflow + interflow_leak + precip

   Solve: (dt/dx)*Q + alpha*Q^beta = RHS
   Using Newton-Raphson iteration
   ```

4. **Update Channel State**:
   ```
   previous_channel_flow = Q
   discharge = Q * channel_width
   ```

### Newton-Raphson Solution

The nonlinear kinematic wave equation is solved iteratively:

```cpp
double SolveKinematicWave(double rhs, double alpha, double beta, double dtdx) {
    double q = rhs / (dtdx + alpha);  // Initial guess

    for (int i = 0; i < 10; i++) {
        double f = dtdx * q + alpha * pow(q, beta) - rhs;
        double fp = dtdx + alpha * beta * pow(q, beta - 1);
        q = q - f / fp;
        if (q < 0) q = 0;
    }

    return q;
}
```

### Cell Type Classification

Cells are classified based on flow accumulation:

```
If flowAccumulation >= TH:
    cell_type = CHANNEL
Else:
    cell_type = HILLSLOPE
```

Channel cells use different alpha/beta parameters and accumulate flow from surrounding hillslope cells.

### State Persistence

The Kinematic Wave model supports saving and loading states for:
- Warm starts (continuing from previous simulation)
- Data assimilation
- Checkpoint/restart capability

States are stored as GeoTIFF grids with the same resolution as the model grid.

### Strengths and Limitations

**Strengths:**
- More physically based than linear reservoir
- Captures nonlinear flow dynamics
- Better representation of flood wave propagation
- Separate treatment of hillslope and channel processes

**Limitations:**
- More computationally intensive
- Requires more parameters
- Kinematic assumption invalid for:
  - Backwater effects
  - Very mild slopes
  - Tidal influences
- Iterative solution may not converge for extreme conditions

---

## Comparison of Routing Models

| Aspect | Linear Reservoir | Kinematic Wave |
|--------|-----------------|----------------|
| **Physical Basis** | Conceptual | Semi-physical |
| **Computation Speed** | Fast | Moderate |
| **Parameter Count** | 8 | 7 |
| **State Variables** | 2 reservoirs | 3 states |
| **Channel/Hillslope** | Same treatment | Different treatment |
| **Nonlinearity** | Linear outflow | Nonlinear (power law) |
| **Backwater Effects** | Not captured | Not captured |
| **Best For** | Real-time forecasting | Process studies |
| **Initialization** | Simple (reservoir levels) | Requires previous flows |

---

## Integration with Water Balance Models

Both routing models interface with water balance models through a common interface:

```cpp
// Water balance model outputs
float fastFlow;   // Surface runoff (mm/hr)
float slowFlow;   // Interflow/baseflow (mm/hr)

// Routing model receives these as inputs
routingModel->Route(fastFlow, slowFlow);

// Routing model outputs
float discharge;  // Streamflow at gauges (m^3/s or cfs)
```

### Flow Path Calculation

Both routing models use the D8 flow direction algorithm:
1. Each cell has a single downstream cell
2. Flow follows steepest descent
3. Cells are processed in topological order (upstream first)

---

## Configuration Example

```xml
<Route>
    <Type>LR</Type>  <!-- or "KW" for Kinematic Wave -->
    <Parameters>
        <Gauge name="outlet">
            <COEM>0.05</COEM>
            <RIVER>0.03</RIVER>
            <UNDER>0.01</UNDER>
            <LEAKO>0.5</LEAKO>
            <LEAKI>0.3</LEAKI>
            <TH>100</TH>
            <ISO>0.0</ISO>
            <ISU>10.0</ISU>
        </Gauge>
    </Parameters>
</Route>
```

---

## References

1. Flamig, Z. L., et al. (2020). "The Ensemble Framework For Flash Flood Forecasting (EF5)." *Journal of Hydrometeorology*.

2. Singh, V. P. (1996). *Kinematic Wave Modeling in Water Resources: Surface-Water Hydrology*. John Wiley & Sons.

3. Chow, V. T., Maidment, D. R., & Mays, L. W. (1988). *Applied Hydrology*. McGraw-Hill.

4. Henderson, F. M. (1966). *Open Channel Flow*. Macmillan.
