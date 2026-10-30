## RDNA 3 (RX 7900)

### Material shaders

| Name          | Permutation               | Stages  | VS/MS VGPR | VS/MS SGPR | LDS           | PS VGPR | PS SGPR |
| ------------- | ------------------------- | ------- | ---------- | ---------- | ------------- | ------- | ------- |
| Placeholder   | `DEFAULT`                 | MS + PS | 47/256     | 89/106     | 10.00/64.00Kb | 71/256  | 88/106  |
| Placeholder   | `DEPTH_ONLY`              | MS      | 47/256     | 89/106     | 10.00/64.00Kb | -       | -       |
| Placeholder   | `ALPHA_TEST`              | MS + PS | 47/256     | 89/106     | 10.00/64.00Kb | 71/256  | 88/106  |
| Placeholder   | `DEPTH_ONLY` `ALPHA_TEST` | MS + PS | 33/256     | 93/106     | 8.50/64.00Kb  | 13/256  | 33/106  |
| PrincipledPBR | `DEFAULT`                 | MS + PS | 47/256     | 89/106     | 10.00/64.00Kb | 89/256  | 92/106  |
| PrincipledPBR | `DEPTH_ONLY`              | MS      | 47/256     | 89/106     | 10.00/64.00Kb | -       | -       |
| PrincipledPBR | `ALPHA_TEST`              | MS + PS | 47/256     | 89/106     | 10.00/64.00Kb | 89/256  | 92/106  |
| PrincipledPBR | `DEPTH_ONLY` `ALPHA_TEST` | MS + PS | 38/256     | 93/106     | 9.50/64.00Kb  | 18/256  | 42/106  |
| SyntyPBR      | `DEFAULT`                 | MS + PS | 47/256     | 89/106     | 10.00/64.00Kb | 84/256  | 88/106  |
| SyntyPBR      | `DEPTH_ONLY`              | MS      | 47/256     | 89/106     | 10.00/64.00Kb | -       | -       |
| SyntyPBR      | `ALPHA_TEST`              | MS + PS | 47/256     | 89/106     | 10.00/64.00Kb | 84/256  | 88/106  |
| SyntyPBR      | `DEPTH_ONLY` `ALPHA_TEST` | MS + PS | 38/256     | 93/106     | 9.50/64.00Kb  | 18/256  | 42/106  |

### Surface shaders

| Name                           | Stages  | VS/MS VGPR | VS/MS SGPR | LDS           | PS VGPR | PS SGPR |
| ------------------------------ | ------- | ---------- | ---------- | ------------- | ------- | ------- |
| BilateralUpsample              | VS + PS | 12/256     | 26/106     | -             | 12/256  | 32/106  |
| DebugDraw                      | MS + PS | 72/256     | 66/106     | 15.50/64.00Kb | 17/256  | 22/106  |
| DebugDrawMesh                  | MS + PS | 37/256     | 88/106     | 8.00/64.00Kb  | 11/256  | 19/106  |
| DepthPyramidReduction          | VS + PS | 12/256     | 24/106     | -             | 6/256   | 24/106  |
| Downsample                     | VS + PS | 12/256     | 22/106     | -             | 5/256   | 20/106  |
| EditorOutline_Composite        | VS + PS | 12/256     | 26/106     | -             | 7/256   | 24/106  |
| EditorOutline_Initialize       | VS + PS | 9/256      | 26/106     | -             | 9/256   | 24/106  |
| EditorOutline_JumpFlood        | VS + PS | 12/256     | 26/106     | -             | 29/256  | 24/106  |
| OITResolve                     | VS + PS | 9/256      | 22/106     | -             | 4/256   | 16/106  |
| CubemapDownsample              | VS + PS | 12/256     | 24/106     | -             | 7/256   | 32/106  |
| PostProcess                    | VS + PS | 12/256     | 22/106     | -             | 6/256   | 28/106  |
| Imgui                          | VS + PS | 16/256     | 24/106     | -             | 11/256  | 18/106  |
| SMAA_BlendingWeightCalculation | VS + PS | 26/256     | 26/106     | -             | 30/256  | 38/106  |
| SMAA_EdgeDetection             | VS + PS | 25/256     | 26/106     | -             | 25/256  | 24/106  |
| SMAA_NeighborhoodBlending      | VS + PS | 13/256     | 26/106     | -             | 14/256  | 26/106  |

### Compute shaders

| Name                            | VGPR   | SGPR   | Occupancy   | LDS          | Scratch   |
| ------------------------------- | ------ | ------ | ----------- | ------------ | --------- |
| DebugDrawResolve                | 9/256  | 28/106 | 96.48%      | 0.00/64.00Kb | 0B        |
| FFX_ParallelSort_Count          | 12/256 | 20/106 | 95.31%      | 8.00/64.00Kb | 0B        |
| FFX_ParallelSort_Reduce         | 8/256  | 20/106 | 96.88%      | 0.50/64.00Kb | 0B        |
| FFX_ParallelSort_Scan           | 15/256 | 20/106 | 94.14%      | 2.50/64.00Kb | 0B        |
| FFX_ParallelSort_ScanAdd        | 16/256 | 24/106 | 93.75%      | 2.50/64.00Kb | 0B        |
| FFX_ParallelSort_Scatter        | 20/256 | 24/106 | 92.19%      | 2.00/64.00Kb | 0B        |
| FFX_ParallelSort_ScatterPayload | 24/256 | 32/106 | 90.62%      | 2.00/64.00Kb | 0B        |
| FFX_ParallelSort_SetupIndirect  | 19/256 | 32/106 | 92.58%      | 0.00/64.00Kb | 0B        |
| Denoise                         | 42/256 | 48/106 | 83.59%      | 0.00/64.00Kb | 0B        |
| MainPass                        | 53/256 | 48/106 | 79.30%      | 0.00/64.00Kb | 0B        |
| PrefilterDepth                  | 30/256 | 48/106 | 88.28%      | 0.50/64.00Kb | 0B        |
| InstancePickingResolve          | 9/256  | 20/106 | 96.48%      | 0.00/64.00Kb | 0B        |
| IrradianceSHProject             | 46/256 | 42/106 | 82.03%      | 3.00/64.00Kb | 0B        |
| ProbeTableRadiance              | 28/256 | 52/106 | 89.06%      | 0.00/64.00Kb | 0B        |
| CullingCompaction               | 7/256  | 16/106 | 97.27%      | 0.00/64.00Kb | 0B        |
| CullingArgumentGeneration       | 17/256 | 25/106 | 93.36%      | 0.00/64.00Kb | 0B        |
| DrawCompaction                  | 7/256  | 40/106 | 97.27%      | 0.00/64.00Kb | 0B        |
| DrawArgumentGeneration          | 15/256 | 52/106 | 94.14%      | 0.00/64.00Kb | 0B        |
| ClusterCulling                  | 38/256 | 64/106 | 85.16%      | 0.00/64.00Kb | 0B        |
| ClusterToInstanceUpdate         | 5/256  | 16/106 | 98.05%      | 0.00/64.00Kb | 0B        |
| InstanceCulling                 | 34/256 | 60/106 | 86.72%      | 0.00/64.00Kb | 0B        |
| LightCulling_CullLights         | 41/256 | 66/106 | 83.98%      | 0.50/64.00Kb | 0B        |
| InitializeInstances             | 18/256 | 24/106 | 92.97%      | 0.00/64.00Kb | 0B        |
| UpdateInstances                 | 94/256 | 28/106 | 63.28%      | 0.00/64.00Kb | 0B        |

## RDNA 3.5 (Radeon 890M)

### Material shaders

| Name          | Permutation               | Stages  | VS/MS VGPR | VS/MS SGPR | LDS           | PS VGPR | PS SGPR |
| ------------- | ------------------------- | ------- | ---------- | ---------- | ------------- | ------- | ------- |
| Placeholder   | `DEFAULT`                 | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 72/256  | 88/106  |
| Placeholder   | `DEPTH_ONLY`              | MS      | 47/256     | 93/106     | 10.00/64.00Kb | -       | -       |
| Placeholder   | `ALPHA_TEST`              | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 72/256  | 88/106  |
| Placeholder   | `DEPTH_ONLY` `ALPHA_TEST` | MS + PS | 33/256     | 93/106     | 8.50/64.00Kb  | 13/256  | 33/106  |
| PrincipledPBR | `DEFAULT`                 | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 96/256  | 92/106  |
| PrincipledPBR | `DEPTH_ONLY`              | MS      | 47/256     | 93/106     | 10.00/64.00Kb | -       | -       |
| PrincipledPBR | `ALPHA_TEST`              | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 96/256  | 92/106  |
| PrincipledPBR | `DEPTH_ONLY` `ALPHA_TEST` | MS + PS | 38/256     | 93/106     | 9.50/64.00Kb  | 18/256  | 42/106  |
| SyntyPBR      | `DEFAULT`                 | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 72/256  | 88/106  |
| SyntyPBR      | `DEPTH_ONLY`              | MS      | 47/256     | 93/106     | 10.00/64.00Kb | -       | -       |
| SyntyPBR      | `ALPHA_TEST`              | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 72/256  | 88/106  |
| SyntyPBR      | `DEPTH_ONLY` `ALPHA_TEST` | MS + PS | 38/256     | 93/106     | 9.50/64.00Kb  | 18/256  | 42/106  |

### Surface shaders

| Name                           | Stages  | VS/MS VGPR | VS/MS SGPR | LDS           | PS VGPR | PS SGPR |
| ------------------------------ | ------- | ---------- | ---------- | ------------- | ------- | ------- |
| BilateralUpsample              | VS + PS | 12/256     | 26/106     | -             | 12/256  | 32/106  |
| DebugDraw                      | MS + PS | 72/256     | 66/106     | 15.50/64.00Kb | 17/256  | 22/106  |
| DebugDrawMesh                  | MS + PS | 37/256     | 88/106     | 8.00/64.00Kb  | 11/256  | 19/106  |
| DepthPyramidReduction          | VS + PS | 12/256     | 24/106     | -             | 6/256   | 24/106  |
| Downsample                     | VS + PS | 12/256     | 22/106     | -             | 5/256   | 20/106  |
| EditorOutline_Composite        | VS + PS | 12/256     | 26/106     | -             | 7/256   | 24/106  |
| EditorOutline_Initialize       | VS + PS | 9/256      | 26/106     | -             | 9/256   | 24/106  |
| EditorOutline_JumpFlood        | VS + PS | 12/256     | 26/106     | -             | 28/256  | 24/106  |
| OITResolve                     | VS + PS | 9/256      | 22/106     | -             | 4/256   | 16/106  |
| CubemapDownsample              | VS + PS | 12/256     | 24/106     | -             | 7/256   | 32/106  |
| PostProcess                    | VS + PS | 12/256     | 22/106     | -             | 6/256   | 28/106  |
| Imgui                          | VS + PS | 16/256     | 24/106     | -             | 11/256  | 18/106  |
| SMAA_BlendingWeightCalculation | VS + PS | 26/256     | 26/106     | -             | 30/256  | 38/106  |
| SMAA_EdgeDetection             | VS + PS | 25/256     | 26/106     | -             | 25/256  | 24/106  |
| SMAA_NeighborhoodBlending      | VS + PS | 13/256     | 26/106     | -             | 14/256  | 26/106  |

### Compute shaders

| Name                            | VGPR   | SGPR   | Occupancy   | LDS          | Scratch   |
| ------------------------------- | ------ | ------ | ----------- | ------------ | --------- |
| DebugDrawResolve                | 9/256  | 28/106 | 96.48%      | 0.00/64.00Kb | 0B        |
| FFX_ParallelSort_Count          | 12/256 | 20/106 | 95.31%      | 8.00/64.00Kb | 0B        |
| FFX_ParallelSort_Reduce         | 8/256  | 20/106 | 96.88%      | 0.50/64.00Kb | 0B        |
| FFX_ParallelSort_Scan           | 15/256 | 20/106 | 94.14%      | 2.50/64.00Kb | 0B        |
| FFX_ParallelSort_ScanAdd        | 16/256 | 24/106 | 93.75%      | 2.50/64.00Kb | 0B        |
| FFX_ParallelSort_Scatter        | 20/256 | 24/106 | 92.19%      | 2.00/64.00Kb | 0B        |
| FFX_ParallelSort_ScatterPayload | 24/256 | 32/106 | 90.62%      | 2.00/64.00Kb | 0B        |
| FFX_ParallelSort_SetupIndirect  | 19/256 | 33/106 | 92.58%      | 0.00/64.00Kb | 0B        |
| Denoise                         | 41/256 | 48/106 | 83.98%      | 0.00/64.00Kb | 0B        |
| MainPass                        | 51/256 | 48/106 | 80.08%      | 0.00/64.00Kb | 0B        |
| PrefilterDepth                  | 32/256 | 48/106 | 87.50%      | 0.50/64.00Kb | 0B        |
| InstancePickingResolve          | 9/256  | 20/106 | 96.48%      | 0.00/64.00Kb | 0B        |
| IrradianceSHProject             | 48/256 | 42/106 | 81.25%      | 3.00/64.00Kb | 0B        |
| ProbeTableRadiance              | 28/256 | 48/106 | 89.06%      | 0.00/64.00Kb | 0B        |
| CullingCompaction               | 7/256  | 16/106 | 97.27%      | 0.00/64.00Kb | 0B        |
| CullingArgumentGeneration       | 17/256 | 25/106 | 93.36%      | 0.00/64.00Kb | 0B        |
| DrawCompaction                  | 7/256  | 40/106 | 97.27%      | 0.00/64.00Kb | 0B        |
| DrawArgumentGeneration          | 15/256 | 52/106 | 94.14%      | 0.00/64.00Kb | 0B        |
| ClusterCulling                  | 38/256 | 64/106 | 85.16%      | 0.00/64.00Kb | 0B        |
| ClusterToInstanceUpdate         | 5/256  | 16/106 | 98.05%      | 0.00/64.00Kb | 0B        |
| InstanceCulling                 | 34/256 | 60/106 | 86.72%      | 0.00/64.00Kb | 0B        |
| LightCulling_CullLights         | 30/256 | 72/106 | 88.28%      | 0.50/64.00Kb | 0B        |
| InitializeInstances             | 18/256 | 24/106 | 92.97%      | 0.00/64.00Kb | 0B        |
| UpdateInstances                 | 94/256 | 28/106 | 63.28%      | 0.00/64.00Kb | 0B        |

## RDNA 4.0 (RX 9060)

### Material shaders

| Name          | Permutation               | Stages  | VS/MS VGPR | VS/MS SGPR | LDS           | PS VGPR | PS SGPR |
| ------------- | ------------------------- | ------- | ---------- | ---------- | ------------- | ------- | ------- |
| Placeholder   | `DEFAULT`                 | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 84/256  | 88/106  |
| Placeholder   | `DEPTH_ONLY`              | MS      | 47/256     | 93/106     | 10.00/64.00Kb | -       | -       |
| Placeholder   | `ALPHA_TEST`              | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 84/256  | 88/106  |
| Placeholder   | `DEPTH_ONLY` `ALPHA_TEST` | MS + PS | 36/256     | 93/106     | 8.50/64.00Kb  | 13/256  | 34/106  |
| PrincipledPBR | `DEFAULT`                 | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 89/256  | 92/106  |
| PrincipledPBR | `DEPTH_ONLY`              | MS      | 47/256     | 93/106     | 10.00/64.00Kb | -       | -       |
| PrincipledPBR | `ALPHA_TEST`              | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 89/256  | 92/106  |
| PrincipledPBR | `DEPTH_ONLY` `ALPHA_TEST` | MS + PS | 38/256     | 93/106     | 9.50/64.00Kb  | 18/256  | 42/106  |
| SyntyPBR      | `DEFAULT`                 | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 84/256  | 88/106  |
| SyntyPBR      | `DEPTH_ONLY`              | MS      | 47/256     | 93/106     | 10.00/64.00Kb | -       | -       |
| SyntyPBR      | `ALPHA_TEST`              | MS + PS | 47/256     | 93/106     | 10.00/64.00Kb | 84/256  | 88/106  |
| SyntyPBR      | `DEPTH_ONLY` `ALPHA_TEST` | MS + PS | 38/256     | 93/106     | 9.50/64.00Kb  | 18/256  | 42/106  |

### Surface shaders

| Name                           | Stages  | VS/MS VGPR | VS/MS SGPR | LDS           | PS VGPR | PS SGPR |
| ------------------------------ | ------- | ---------- | ---------- | ------------- | ------- | ------- |
| BilateralUpsample              | VS + PS | 11/256     | 26/106     | -             | 12/256  | 32/106  |
| DebugDraw                      | MS + PS | 72/256     | 66/106     | 15.50/64.00Kb | 19/256  | 22/106  |
| DebugDrawMesh                  | MS + PS | 36/256     | 88/106     | 8.00/64.00Kb  | 11/256  | 19/106  |
| DepthPyramidReduction          | VS + PS | 11/256     | 24/106     | -             | 6/256   | 24/106  |
| Downsample                     | VS + PS | 11/256     | 22/106     | -             | 5/256   | 20/106  |
| EditorOutline_Composite        | VS + PS | 11/256     | 26/106     | -             | 7/256   | 20/106  |
| EditorOutline_Initialize       | VS + PS | 5/256      | 26/106     | -             | 9/256   | 24/106  |
| EditorOutline_JumpFlood        | VS + PS | 11/256     | 26/106     | -             | 29/256  | 20/106  |
| OITResolve                     | VS + PS | 5/256      | 22/106     | -             | 4/256   | 12/106  |
| CubemapDownsample              | VS + PS | 11/256     | 24/106     | -             | 7/256   | 32/106  |
| PostProcess                    | VS + PS | 11/256     | 22/106     | -             | 6/256   | 28/106  |
| Imgui                          | VS + PS | 13/256     | 24/106     | -             | 12/256  | 18/106  |
| SMAA_BlendingWeightCalculation | VS + PS | 28/256     | 26/106     | -             | 30/256  | 38/106  |
| SMAA_EdgeDetection             | VS + PS | 24/256     | 26/106     | -             | 23/256  | 18/106  |
| SMAA_NeighborhoodBlending      | VS + PS | 13/256     | 26/106     | -             | 17/256  | 22/106  |

### Compute shaders

| Name                            | VGPR   | SGPR   | Occupancy   | LDS          | Scratch   |
| ------------------------------- | ------ | ------ | ----------- | ------------ | --------- |
| DebugDrawResolve                | 9/256  | 24/106 | 96.48%      | 0.00/64.00Kb | 0B        |
| FFX_ParallelSort_Count          | 12/256 | 16/106 | 95.31%      | 8.00/64.00Kb | 0B        |
| FFX_ParallelSort_Reduce         | 8/256  | 20/106 | 96.88%      | 0.50/64.00Kb | 0B        |
| FFX_ParallelSort_Scan           | 15/256 | 20/106 | 94.14%      | 2.50/64.00Kb | 0B        |
| FFX_ParallelSort_ScanAdd        | 16/256 | 24/106 | 93.75%      | 2.50/64.00Kb | 0B        |
| FFX_ParallelSort_Scatter        | 20/256 | 24/106 | 92.19%      | 1.56/64.00Kb | 0B        |
| FFX_ParallelSort_ScatterPayload | 24/256 | 32/106 | 90.62%      | 1.56/64.00Kb | 0B        |
| FFX_ParallelSort_SetupIndirect  | 20/256 | 34/106 | 92.19%      | 0.00/64.00Kb | 0B        |
| Denoise                         | 43/256 | 40/106 | 83.20%      | 0.00/64.00Kb | 0B        |
| MainPass                        | 51/256 | 44/106 | 80.08%      | 0.00/64.00Kb | 0B        |
| PrefilterDepth                  | 15/256 | 36/106 | 94.14%      | 0.25/64.00Kb | 0B        |
| InstancePickingResolve          | 9/256  | 20/106 | 96.48%      | 0.00/64.00Kb | 0B        |
| IrradianceSHProject             | 43/256 | 42/106 | 83.20%      | 3.00/64.00Kb | 0B        |
| ProbeTableRadiance              | 28/256 | 52/106 | 89.06%      | 0.00/64.00Kb | 0B        |
| CullingCompaction               | 8/256  | 16/106 | 96.88%      | 0.00/64.00Kb | 0B        |
| CullingArgumentGeneration       | 13/256 | 24/106 | 94.92%      | 0.00/64.00Kb | 0B        |
| DrawCompaction                  | 7/256  | 44/106 | 97.27%      | 0.00/64.00Kb | 0B        |
| DrawArgumentGeneration          | 13/256 | 54/106 | 94.92%      | 0.00/64.00Kb | 0B        |
| ClusterCulling                  | 39/256 | 60/106 | 84.77%      | 0.00/64.00Kb | 0B        |
| ClusterToInstanceUpdate         | 5/256  | 20/106 | 98.05%      | 0.00/64.00Kb | 0B        |
| InstanceCulling                 | 34/256 | 60/106 | 86.72%      | 0.00/64.00Kb | 0B        |
| LightCulling_CullLights         | 30/256 | 70/106 | 88.28%      | 0.14/64.00Kb | 0B        |
| InitializeInstances             | 16/256 | 24/106 | 93.75%      | 0.00/64.00Kb | 0B        |
| UpdateInstances                 | 94/256 | 28/106 | 63.28%      | 0.00/64.00Kb | 0B        |
