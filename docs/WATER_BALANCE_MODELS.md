# Water Balance Models in EF5

This document provides detailed technical documentation for the water balance models implemented in the Ensemble Framework For Flash Flood Forecasting (EF5). Water balance models simulate the vertical water budget at each grid cell, partitioning precipitation into evapotranspiration, soil moisture storage, and runoff components.

## Overview

EF5 implements four water balance models with varying complexity:

| Model | Full Name | Complexity | States | Parameters | Type |
|-------|-----------|------------|--------|------------|------|
| **SAC** | Sacramento Soil Moisture Accounting | High | 6 | 23 | Distributed |
| **CREST** | Coupled Routing and Excess Storage | Moderate | 1 | 6 | Distributed |
| **HyMOD** | Hydrological Model | Moderate | 4-6 | 9 | Lumped |
| **HP** | Hydrophobic | Simple | 0 | 2 | Distributed |

All models output two flow components:
- **Fast Flow (Surface Runoff)**: Water that flows quickly over or near the surface
- **Slow Flow (Interflow/Baseflow)**: Water that moves slowly through subsurface pathways

---

## Sacramento Soil Moisture Accounting Model (SAC-SMA)

**Source Files:** `src/SAC.h`, `src/SAC.cpp`

### Overview

The Sacramento model is a conceptual rainfall-runoff model developed by the U.S. National Weather Service. It is one of the most widely used operational hydrologic models in the United States. SAC-SMA represents the soil column as two zones (upper and lower), each with tension water and free water storage components.

### Conceptual Structure

```
                    Precipitation
                         |
                         v
    +--------------------------------------------+
    |              UPPER ZONE                     |
    |  +------------------+  +------------------+ |
    |  | Tension Water    |  | Free Water       | |
    |  | (UZTWC/UZTWM)    |  | (UZFWC/UZFWM)    | |
    |  +------------------+  +------------------+ |
    |         |                    |             |
    |         v                    v             |
    |        ET              Interflow --------> Slow Flow
    |                              |             |
    +------------------------------|-------------+
                                   | Percolation
                                   v
    +--------------------------------------------+
    |              LOWER ZONE                     |
    |  +------------------+                       |
    |  | Tension Water    |                       |
    |  | (LZTWC/LZTWM)    |                       |
    |  +--------+---------+                       |
    |           |                                 |
    |  +--------v---------+  +------------------+ |
    |  | Supplemental     |  | Primary          | |
    |  | Free Water       |  | Free Water       | |
    |  | (LZFSC/LZFSM)    |  | (LZFPC/LZFPM)    | |
    |  +------------------+  +------------------+ |
    |         |                    |             |
    |         v                    v             |
    |    Baseflow (fast)     Baseflow (slow) --> Slow Flow
    +--------------------------------------------+

    +--------------------------------------------+
    |         IMPERVIOUS AREAS                   |
    |  - Direct Runoff (PCTIM)                   |
    |  - Additional Impervious (ADIMP)      ---> Fast Flow
    +--------------------------------------------+
```

### Parameters

| Parameter | Name | Description | Typical Range | Units |
|-----------|------|-------------|---------------|-------|
| `UZTWM` | Upper Zone Tension Water Max | Maximum upper zone tension water storage | 10 - 300 | mm |
| `UZFWM` | Upper Zone Free Water Max | Maximum upper zone free water storage | 10 - 150 | mm |
| `UZK` | Upper Zone Coefficient | Upper zone free water lateral outflow rate | 0.1 - 0.75 | 1/day |
| `PCTIM` | Percent Impervious | Permanent impervious area fraction | 0.0 - 0.1 | - |
| `ADIMP` | Additional Impervious | Additional impervious area when saturated | 0.0 - 0.4 | - |
| `RIVA` | Riparian Area | Riparian vegetation area fraction | 0.0 - 0.2 | - |
| `ZPERC` | Percolation Demand | Maximum percolation rate multiplier | 5 - 350 | - |
| `REXP` | Percolation Exponent | Percolation equation exponent | 1.0 - 5.0 | - |
| `LZTWM` | Lower Zone Tension Water Max | Maximum lower zone tension water storage | 50 - 500 | mm |
| `LZFSM` | Lower Zone Supplemental Max | Maximum lower zone supplemental free water | 10 - 400 | mm |
| `LZFPM` | Lower Zone Primary Max | Maximum lower zone primary free water | 50 - 1000 | mm |
| `LZSK` | Lower Zone Supplemental K | Lower zone supplemental drainage rate | 0.01 - 0.35 | 1/day |
| `LZPK` | Lower Zone Primary K | Lower zone primary drainage rate | 0.001 - 0.05 | 1/day |
| `PFREE` | Percolation to Free | Fraction of percolation going to free water | 0.0 - 0.8 | - |
| `SIDE` | Side Flow Ratio | Ratio of deep percolation to streamflow | 0.0 - 0.5 | - |
| `RSERV` | Lower Zone Minimum | Minimum lower zone storage as fraction | 0.0 - 0.4 | - |
| `UZTWC` | Initial UZTW Content | Initial upper zone tension water | 0 - UZTWM | mm |
| `UZFWC` | Initial UZFW Content | Initial upper zone free water | 0 - UZFWM | mm |
| `LZTWC` | Initial LZTW Content | Initial lower zone tension water | 0 - LZTWM | mm |
| `LZFSC` | Initial LZFS Content | Initial lower zone supplemental free water | 0 - LZFSM | mm |
| `LZFPC` | Initial LZFP Content | Initial lower zone primary free water | 0 - LZFPM | mm |
| `ADIMC` | Initial ADIM Content | Initial additional impervious area content | 0 - (UZTWM+LZTWM) | mm |

### State Variables

| State | Description | Bounds |
|-------|-------------|--------|
| `UZTWC` | Upper zone tension water content | 0 to UZTWM |
| `UZFWC` | Upper zone free water content | 0 to UZFWM |
| `LZTWC` | Lower zone tension water content | 0 to LZTWM |
| `LZFSC` | Lower zone supplemental free water content | 0 to LZFSM |
| `LZFPC` | Lower zone primary free water content | 0 to LZFPM |
| `ADIMC` | Additional impervious area content | 0 to (UZTWM+LZTWM) |

### Algorithm (`WaterBalanceInt`)

The SAC algorithm operates in two main phases:

#### Phase 1: Evapotranspiration

```
1. Calculate potential ET demand: E1 = PET for this timestep

2. ET from upper zone tension water:
   E1 = min(E1, UZTWC)
   UZTWC = UZTWC - E1
   RED = E1_original - E1  // Residual demand

3. ET from upper zone free water (if demand remains):
   E2 = min(RED, UZFWC)
   UZFWC = UZFWC - E2
   RED = RED - E2

4. ET from lower zone tension water (if demand remains):
   SAVED = RSERV * (LZFPM + LZFSM)  // Protected storage
   E3 = RED * (LZTWC / (UZTWM + LZTWM))
   LZTWC = LZTWC - E3

5. ET from additional impervious area:
   E5 = E1 * (ADIMC / UZTWM)
   ADIMC = ADIMC - E5 - (E1 - E3) * (ADIMC / LZTWM)

6. Rebalance upper zone storages to maintain equilibrium
```

#### Phase 2: Runoff Generation

```
1. Calculate excess water available for runoff:
   TWX = precip - (UZTWM - UZTWC)  // Excess above tension capacity

2. If TWX > 0 (excess exists):
   // Fill upper zone tension water
   UZTWC = UZTWM

   // Add to additional impervious area
   ADIMC = ADIMC + precip - TWX

3. Determine sub-timestep increments:
   NINC = floor(1 + 0.2 * (UZFWC + TWX))
   DINC = TWX / NINC

4. For each sub-increment:
   a. Calculate baseflow from lower zone:
      BF_primary = LZFPC * (1 - exp(-LZPK * dt))
      BF_supplemental = LZFSC * (1 - exp(-LZSK * dt))
      LZFPC = LZFPC - BF_primary
      LZFSC = LZFSC - BF_supplemental

   b. Calculate percolation from upper to lower zone:
      DEFR = 1 - (LZTWC + LZFPC + LZFSC) / (LZTWM + LZFPM + LZFSM)
      PERC = UZFWC * (1 + ZPERC * DEFR^REXP)
      PERC = min(PERC, UZFWC)

   c. Distribute percolation:
      // To lower zone tension water
      PERCT = PERC * (1 - PFREE) * min(1, (LZTWM - LZTWC) / available)
      LZTWC = LZTWC + PERCT

      // To lower zone free water (split between primary and supplemental)
      PERCF = PERC - PERCT
      // Distribute based on relative deficits

   d. Calculate interflow:
      INTERFLOW = UZFWC * UZK

   e. Calculate surface runoff:
      SURF = (DINC - infiltration) * (1 - PCTIM - ADIMP)

   f. Calculate direct runoff from impervious areas:
      DIRECT = DINC * PCTIM
      ADDRO = DINC * ADIMP * (ADIMC / (UZTWM + LZTWM))^2

5. Total outputs:
   Fast Flow (SURF) = Surface runoff + Direct runoff + Additional impervious runoff
   Slow Flow (GRND) = Interflow + Primary baseflow + Supplemental baseflow
```

### Key Equations

**Percolation Rate:**
```
PERC = PERCM * (UZFWC/UZFWM) * [1 + ZPERC * DEFR^REXP]

Where:
- PERCM = maximum percolation rate
- DEFR = lower zone deficiency ratio = 1 - (LZTWC+LZFPC+LZFSC)/(LZTWM+LZFPM+LZFSM)
- ZPERC, REXP = percolation shape parameters
```

**Baseflow Recession:**
```
BF = Storage * [1 - exp(-K * dt)]

Where:
- K = recession coefficient (LZPK or LZSK)
- dt = time increment
```

**Additional Impervious Runoff:**
```
ADDRO = Precip * ADIMP * (ADIMC / (UZTWM + LZTWM))^2
```

### Strengths and Limitations

**Strengths:**
- Well-tested operational model
- Physically meaningful parameters
- Good representation of baseflow processes
- Handles dry and wet antecedent conditions well

**Limitations:**
- Many parameters require calibration
- Complex parameter interactions
- May be over-parameterized for some applications

---

## CREST Model

**Source Files:** `src/CRESTModel.h`, `src/CRESTModel.cpp`

### Overview

The Coupled Routing and Excess Storage (CREST) model is a distributed hydrologic model developed specifically for flash flood prediction. It uses a simplified soil moisture representation with a variable infiltration capacity approach derived from the VIC model.

### Conceptual Structure

```
              Precipitation
                   |
                   v
    +------------------------------+
    |        ATMOSPHERE            |
    |   +-----------------------+  |
    |   | PET * KE (ET demand) |  |
    |   +-----------------------+  |
    +--------------|---------------+
                   v
    +------------------------------+
    |      LAND SURFACE            |
    |  +---------+  +----------+   |
    |  |Impervious|  | Pervious |   |
    |  | (IM)     |  | (1-IM)   |   |
    |  +----+----+  +-----+----+   |
    |       |             |        |
    |       v             v        |
    |  Direct RO    Infiltration   |
    +--------------|---------------+
                   v
    +------------------------------+
    |         SOIL ZONE            |
    |  +------------------------+  |
    |  |    Soil Moisture (SM)  |  |
    |  |    Capacity: WM        |  |
    |  +------------------------+  |
    |         |           |        |
    |         v           v        |
    |     Overflow    Interflow    |
    +--------------|---------------+
                   v
         Fast Flow + Slow Flow
```

### Parameters

| Parameter | Name | Description | Typical Range | Units |
|-----------|------|-------------|---------------|-------|
| `WM` | Water Capacity Max | Maximum soil water storage capacity | 50 - 500 | mm |
| `B` | Shape Parameter | Infiltration capacity distribution shape | 0.1 - 2.0 | - |
| `IM` | Impervious Fraction | Fraction of impervious area | 0.0 - 0.3 | - |
| `KE` | ET Coefficient | Evapotranspiration scaling factor | 0.5 - 1.5 | - |
| `FC` | Infiltration Capacity | Mean infiltration capacity coefficient | 0.5 - 2.0 | - |
| `IWU` | Initial Water Use | Initial soil moisture as % of WM | 0 - 100 | % |

### State Variables

| State | Description | Bounds |
|-------|-------------|--------|
| `SM` | Soil Moisture | 0 to WM |

### Algorithm (`WaterBalanceInt`)

CREST uses a variable infiltration capacity (VIC) approach:

#### Step 1: Preprocessing

```
// Adjust PET by crop coefficient
adjPET = PET * KE

// Direct runoff from impervious area
ROIMP = precip * IM

// Precipitation reaching soil
precipSoil = (precip - adjPET) * (1 - IM)
```

#### Step 2: Wet Condition (precip > PET)

```
// Maximum water holding capacity across distribution
Wmaxm = WM * (1 + B)

// Calculate infiltration capacity at current soil moisture
A = Wmaxm * (1 - (1 - SM/WM)^(1/(1+B)))

// Case 1: Precipitation fills remaining capacity
If (A + precipSoil) >= Wmaxm:
    infiltration = WM - SM
    excess = precipSoil - infiltration

// Case 2: Partial infiltration
Else:
    infiltration = WM * [(1 - A/Wmaxm)^(1+B) - (1 - (A+precipSoil)/Wmaxm)^(1+B)]
    excess = precipSoil - infiltration

// Update soil moisture
SM = SM + infiltration

// Split excess into surface runoff and interflow
// Based on infiltration capacity distribution
interflow = excess * (FC / (FC + 1))
surfaceRunoff = excess - interflow
```

#### Step 3: Dry Condition (precip <= PET)

```
// ET limited by available soil moisture
actualET = min(adjPET, SM + precip)
SM = SM + precip - actualET

// Any excess becomes interflow
If SM > WM:
    interflow = SM - WM
    SM = WM
```

#### Step 4: Output

```
Fast Flow = surfaceRunoff + ROIMP
Slow Flow = interflow
```

### Variable Infiltration Capacity

The VIC approach assumes infiltration capacity varies spatially within each grid cell following a power distribution:

```
Cumulative distribution: F(i) = 1 - (1 - i/im)^B

Where:
- i = infiltration capacity at a point
- im = maximum infiltration capacity = WM * (1 + B)
- B = shape parameter (higher B = more spatial variability)
```

This allows the model to generate runoff even when the average soil moisture is below capacity, representing partial area contribution.

### Strengths and Limitations

**Strengths:**
- Simple with only 6 parameters
- Computationally efficient
- Good for flash flood prediction
- Physically-based infiltration excess mechanism

**Limitations:**
- Single-layer soil representation
- Limited baseflow simulation capability
- May not perform well in dry climates

---

## HyMOD

**Source Files:** `src/HyMOD.h`, `src/HyMOD.cpp`

### Overview

HyMOD is a conceptual lumped rainfall-runoff model that combines a soil moisture module with a series of linear reservoirs for flow routing. Unlike SAC and CREST, HyMOD operates on basin-averaged values rather than individual grid cells.

### Conceptual Structure

```
         Precipitation
              |
              v
    +-------------------+
    |   SOIL MOISTURE   |
    |   (Beta dist.)    |
    |   HUZ, B params   |
    +--------+----------+
             |
             v
    +-------------------+
    |   FLOW SPLITTING  |
    |   ALP fraction    |
    +--------+----------+
             |
     +-------+-------+
     |               |
     v               v
+----------+    +---------+
|  QUICK   |    |  SLOW   |
|  FLOW    |    |  FLOW   |
| NQ tanks |    | 1 tank  |
|  KQ rate |    | KS rate |
+----+-----+    +----+----+
     |               |
     v               v
   Fast Flow      Slow Flow
         \         /
          \       /
           v     v
         Total Q
```

### Parameters

| Parameter | Name | Description | Typical Range | Units |
|-----------|------|-------------|---------------|-------|
| `HUZ` | Upper Zone Height | Maximum soil moisture storage | 50 - 500 | mm |
| `B` | Beta Parameter | Soil moisture distribution shape | 0.1 - 2.0 | - |
| `ALP` | Alpha | Fraction routed to quick flow | 0.0 - 1.0 | - |
| `NQ` | Number Quick Tanks | Number of quick flow reservoirs | 2 - 4 | - |
| `KQ` | Quick Tank Rate | Quick flow reservoir recession | 0.1 - 1.0 | 1/day |
| `KS` | Slow Tank Rate | Slow flow reservoir recession | 0.001 - 0.1 | 1/day |
| `XCUZ` | Initial CUZ | Initial soil moisture fraction | 0 - 1 | - |
| `XQ` | Initial Quick | Initial quick reservoir storage | 0 - 100 | mm |
| `XS` | Initial Slow | Initial slow reservoir storage | 0 - 500 | mm |
| `PRECIP` | Precip Multiplier | Precipitation adjustment factor | 0.8 - 1.2 | - |

### State Variables

| State | Description |
|-------|-------------|
| `XHuz` | Current upper zone height |
| `XCuz` | Current upper zone storage |
| `Xq[NQ]` | Quick flow reservoir storages (array) |
| `Xs` | Slow flow reservoir storage |

### Algorithm (`WaterBalanceInt`)

#### Step 1: Soil Moisture Accounting

```
// Beta distribution parameters
b = log(1 - B/2) / log(0.5)
CPar = HUZ / (1 + b)

// Current storage from height
C = CPar * (1 - (1 - h/HUZ)^(1+b))

// Add precipitation
h_new = h + precip

// Calculate overflow
If h_new > HUZ:
    overflow = h_new - HUZ
    h_new = HUZ
Else:
    overflow = Cmax - C_new
    Where C_new accounts for distribution
```

#### Step 2: Evapotranspiration

```
// ET proportional to relative storage
actualET = PET * (h / HUZ)
h = h - actualET
```

#### Step 3: Flow Splitting

```
// Split overflow between quick and slow pathways
quickInflow = ALP * overflow
slowInflow = (1 - ALP) * overflow
```

#### Step 4: Quick Flow Routing

Quick flow passes through NQ identical linear reservoirs in series (Nash cascade):

```
For each reservoir i from 1 to NQ:
    outflow[i] = KQ * storage[i]
    storage[i] = storage[i] - outflow[i] + inflow[i]
    inflow[i+1] = outflow[i]

Quick Flow = outflow[NQ]
```

#### Step 5: Slow Flow Routing

Slow flow passes through a single linear reservoir:

```
Slow Flow = KS * Xs
Xs = Xs - Slow Flow + slowInflow
```

### Linear Reservoir Theory

Each reservoir follows the linear storage-outflow relationship:

```
Q = K * S

Where:
- Q = outflow rate
- K = recession coefficient
- S = storage

Solution for timestep dt:
Q(t+dt) = Q(t) * exp(-K*dt) + inflow * (1 - exp(-K*dt))
```

The Nash cascade of NQ reservoirs produces a gamma distribution response, smoothing and delaying the input signal.

### Lumped vs Distributed Operation

HyMOD in EF5 operates as a **lumped model**:
- Precipitation is averaged over the basin
- Single set of state variables for entire basin
- Output represents basin outlet discharge

This is different from SAC and CREST which operate on each grid cell independently.

### Strengths and Limitations

**Strengths:**
- Simple and parsimonious (fewer parameters)
- Computationally very fast
- Good performance for many basins
- Clear physical interpretation of parameters

**Limitations:**
- Lumped representation ignores spatial variability
- Limited for large heterogeneous basins
- Single soil layer may miss vertical dynamics
- Fixed recession characteristics

---

## Hydrophobic (HP) Model

**Source Files:** `src/HPModel.h`, `src/HPModel.cpp`

### Overview

The Hydrophobic model is the simplest water balance model in EF5. It acts essentially as a pass-through, splitting precipitation directly into fast and slow flow components without any soil moisture accounting. This is useful for testing routing models in isolation or for situations where soil moisture dynamics are not important.

### Conceptual Structure

```
    Precipitation
         |
         v
    +---------+
    |  SPLIT  |
    |         |
    +---------+
     |       |
     v       v
  Fast    Slow
  Flow    Flow
```

### Parameters

| Parameter | Name | Description | Typical Range | Units |
|-----------|------|-------------|---------------|-------|
| `PRECIP` | Precipitation Multiplier | Scale factor for precipitation | 0.5 - 2.0 | - |
| `SPLIT` | Split Fraction | Fraction going to fast flow | 0.0 - 1.0 | - |

### Algorithm (`WaterBalanceInt`)

```cpp
// Scale precipitation
effectivePrecip = precipitation * PRECIP

// Split into fast and slow components
fastFlow = effectivePrecip * SPLIT
slowFlow = effectivePrecip * (1.0 - SPLIT)

// Convert to flow rates (mm/s)
fastFlow = fastFlow / (stepHours * 3600)
slowFlow = slowFlow / (stepHours * 3600)
```

### Use Cases

1. **Routing Model Testing**: Test routing algorithms without water balance interference
2. **Impervious Urban Areas**: Where all precipitation becomes runoff
3. **Baseline Comparison**: Compare against more complex models
4. **Sensitivity Studies**: Isolate routing model behavior

### Strengths and Limitations

**Strengths:**
- Extremely simple and fast
- No state variables to initialize
- Useful for debugging and testing

**Limitations:**
- No physical realism
- No soil moisture representation
- No evapotranspiration
- Not suitable for actual hydrologic prediction

---

## Model Comparison Summary

| Feature | SAC | CREST | HyMOD | HP |
|---------|-----|-------|-------|-----|
| **Soil Zones** | 2 (Upper + Lower) | 1 | 1 | 0 |
| **State Variables** | 6 | 1 | 4-6 | 0 |
| **Parameters** | 23 | 6 | 9 | 2 |
| **Spatial Type** | Distributed | Distributed | Lumped | Distributed |
| **ET Process** | Multi-zone | Simple | Simple | None |
| **Baseflow** | Dual reservoir | Interflow only | Single reservoir | Direct split |
| **Complexity** | High | Moderate | Moderate | Minimal |
| **Computation** | Moderate | Fast | Very Fast | Very Fast |
| **Best For** | Operational forecasting | Flash floods | Research | Testing |

---

## Integration with Routing Models

All water balance models output to routing models through a common interface:

```cpp
// Water balance calculates excess water
waterBalanceModel->WaterBalance(currentNode, precipRate, petRate);

// Results stored in grid node
float fastFlow = currentNode->excess[LAYER_OVERLAND];   // mm/hr
float slowFlow = currentNode->excess[LAYER_INTERFLOW];  // mm/hr

// Routing model receives these flows
routingModel->Route(fastFlow, slowFlow);
```

### Flow Component Mapping

| Model | Fast Flow Source | Slow Flow Source |
|-------|-----------------|------------------|
| SAC | Surface + Direct + Impervious | Interflow + Baseflow |
| CREST | Surface runoff + Impervious | Interflow |
| HyMOD | Quick flow cascade | Slow flow reservoir |
| HP | SPLIT * Precip | (1-SPLIT) * Precip |

---

## Configuration Example

```xml
<WaterBalance>
    <Type>SAC</Type>  <!-- or CREST, HyMOD, HP -->
    <Parameters>
        <Gauge name="outlet">
            <!-- SAC Parameters -->
            <UZTWM>50.0</UZTWM>
            <UZFWM>40.0</UZFWM>
            <UZK>0.3</UZK>
            <PCTIM>0.01</PCTIM>
            <ADIMP>0.1</ADIMP>
            <RIVA>0.0</RIVA>
            <ZPERC>100.0</ZPERC>
            <REXP>2.0</REXP>
            <LZTWM>150.0</LZTWM>
            <LZFSM>30.0</LZFSM>
            <LZFPM>100.0</LZFPM>
            <LZSK>0.1</LZSK>
            <LZPK>0.01</LZPK>
            <PFREE>0.4</PFREE>
            <SIDE>0.0</SIDE>
            <RSERV>0.3</RSERV>
            <!-- Initial conditions -->
            <UZTWC>25.0</UZTWC>
            <UZFWC>20.0</UZFWC>
            <LZTWC>75.0</LZTWC>
            <LZFSC>15.0</LZFSC>
            <LZFPC>50.0</LZFPC>
            <ADIMC>100.0</ADIMC>
        </Gauge>
    </Parameters>
</WaterBalance>
```

---

## Calibration Considerations

### Parameter Sensitivity

**SAC Model:**
- Most sensitive: UZTWM, UZFWM, LZTWM, ZPERC
- Moderate: UZK, LZPK, LZSK
- Less sensitive: PCTIM, ADIMP, RSERV

**CREST Model:**
- Most sensitive: WM, B, FC
- Moderate: KE, IM
- Less sensitive: IWU (initial condition)

**HyMOD:**
- Most sensitive: HUZ, ALP, KQ
- Moderate: B, KS
- Less sensitive: NQ (often fixed at 3)

### Calibration Strategies

1. **Sequential**: Calibrate water balance first, then routing
2. **Joint**: Calibrate all parameters together
3. **Multi-objective**: Balance multiple performance metrics

### Common Objective Functions

- Nash-Sutcliffe Efficiency (NSE)
- Root Mean Square Error (RMSE)
- Percent Bias (PBIAS)
- Kling-Gupta Efficiency (KGE)

---

## References

1. Burnash, R. J. C., Ferral, R. L., & McGuire, R. A. (1973). "A generalized streamflow simulation system: Conceptual models for digital computers." Joint Federal-State River Forecast Center, Sacramento, CA.

2. Wang, J., et al. (2011). "The coupled routing and excess storage (CREST) distributed hydrological model." *Hydrological Sciences Journal*, 56(1), 84-98.

3. Boyle, D. P. (2000). "Multicriteria calibration of hydrological models." PhD Dissertation, University of Arizona.

4. Moore, R. J. (1985). "The probability-distributed principle and runoff production at point and basin scales." *Hydrological Sciences Journal*, 30(2), 273-297.

5. Flamig, Z. L., et al. (2020). "The Ensemble Framework For Flash Flood Forecasting (EF5)." *Journal of Hydrometeorology*.
