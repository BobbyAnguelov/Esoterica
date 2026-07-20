# ShaderStats

## RDNA 3 (RX 7900)

### Pixel shaders

| Name                           | Permutation            | VGPR   | SGPR   | Occupancy |
| ------------------------------ | ---------------------- | ------ | ------ | --------- |
| BilateralUpsample              |                        | 12/256 | 34/104 | 95.31%    |
| DebugDraw                      |                        | 13/256 | 22/104 | 94.92%    |
| DebugDrawMesh                  |                        | 11/256 | 19/104 | 95.70%    |
| Downsample                     |                        | 5/256  | 22/104 | 98.05%    |
| ComplexSurfacePBR              | `DEFAULT`              | 67/256 | 86/104 | 73.83%    |
| ComplexSurfacePBR              | `ALPHA_TEST`           | 67/256 | 86/104 | 73.83%    |
| ComplexSurfacePBR              | `DEFAULT` `ALPHA_TEST` | 67/256 | 86/104 | 73.83%    |
| DefaultPBR                     | `DEFAULT`              | 52/256 | 86/104 | 79.69%    |
| DefaultPBR                     | `ALPHA_TEST`           | 52/256 | 86/104 | 79.69%    |
| DefaultPBR                     | `DEFAULT` `ALPHA_TEST` | 52/256 | 86/104 | 79.69%    |
| Placeholder                    | `DEFAULT`              | 51/256 | 82/104 | 80.08%    |
| Placeholder                    | `ALPHA_TEST`           | 51/256 | 82/104 | 80.08%    |
| Placeholder                    | `DEFAULT` `ALPHA_TEST` | 51/256 | 82/104 | 80.08%    |
| DefaultColorOnlyPBR            | `DEFAULT`              | 67/256 | 90/104 | 73.83%    |
| DefaultColorOnlyPBR            | `ALPHA_TEST`           | 67/256 | 90/104 | 73.83%    |
| DefaultColorOnlyPBR            | `DEFAULT` `ALPHA_TEST` | 67/256 | 90/104 | 73.83%    |
| OITResolve                     |                        | 4/256  | 16/104 | 98.44%    |
| CubemapDownsample              |                        | 7/256  | 22/104 | 97.27%    |
| IrradianceFiltering            |                        | 34/256 | 20/104 | 86.72%    |
| PrecomputeDFG                  |                        | 13/256 | 8/104  | 94.92%    |
| RadianceFiltering              |                        | 24/256 | 22/104 | 90.62%    |
| PostProcess                    |                        | 6/256  | 30/104 | 97.66%    |
| Imgui                          |                        | 10/256 | 18/104 | 96.09%    |
| SMAA_BlendingWeightCalculation |                        | 30/256 | 40/104 | 88.28%    |
| SMAA_EdgeDetection             |                        | 25/256 | 24/104 | 90.23%    |
| SMAA_NeighborhoodBlending      |                        | 14/256 | 26/104 | 94.53%    |

### Compute shaders

| Name             | VGPR   | SGPR   | Occupancy | LDS          | Scratch |
| ---------------- | ------ | ------ | --------- | ------------ | ------- |
| DebugDrawResolve | 18/256 | 32/104 | 92.97%    | 0.00/64.00Kb | 0Kb     |
| Denoise          | 42/256 | 48/104 | 83.59%    | 0.00/64.00Kb | 0Kb     |
| MainPass         | 53/256 | 48/104 | 79.30%    | 0.00/64.00Kb | 0Kb     |
| PrefilterDepth   | 30/256 | 48/104 | 88.28%    | 0.50/64.00Kb | 0Kb     |
| InstancePicking  | 70/256 | 95/104 | 72.66%    | 5.00/64.00Kb | 0Kb     |
| BucketResolve    | 17/256 | 32/104 | 93.36%    | 0.00/64.00Kb | 0Kb     |
| ClusterCulling   | 38/256 | 60/104 | 85.16%    | 0.00/64.00Kb | 0Kb     |
| InstanceCulling  | 38/256 | 76/104 | 85.16%    | 1.00/64.00Kb | 0Kb     |
| LightCulling     | 53/256 | 58/104 | 79.30%    | 0.50/64.00Kb | 0Kb     |
| WorldUpdate      | 19/256 | 24/104 | 92.58%    | 0.00/64.00Kb | 0Kb     |

## RDNA 3.5 (Radeon 8040S)

### Pixel shaders

| Name                           | Permutation            | VGPR   | SGPR   | Occupancy |
| ------------------------------ | ---------------------- | ------ | ------ | --------- |
| BilateralUpsample              |                        | 12/256 | 34/104 | 95.31%    |
| DebugDraw                      |                        | 13/256 | 22/104 | 94.92%    |
| DebugDrawMesh                  |                        | 11/256 | 19/104 | 95.70%    |
| Downsample                     |                        | 5/256  | 22/104 | 98.05%    |
| ComplexSurfacePBR              | `DEFAULT`              | 72/256 | 86/104 | 71.88%    |
| ComplexSurfacePBR              | `ALPHA_TEST`           | 72/256 | 86/104 | 71.88%    |
| ComplexSurfacePBR              | `DEFAULT` `ALPHA_TEST` | 72/256 | 86/104 | 71.88%    |
| DefaultPBR                     | `DEFAULT`              | 56/256 | 86/104 | 78.12%    |
| DefaultPBR                     | `ALPHA_TEST`           | 56/256 | 86/104 | 78.12%    |
| DefaultPBR                     | `DEFAULT` `ALPHA_TEST` | 56/256 | 86/104 | 78.12%    |
| Placeholder                    | `DEFAULT`              | 49/256 | 82/104 | 80.86%    |
| Placeholder                    | `ALPHA_TEST`           | 49/256 | 82/104 | 80.86%    |
| Placeholder                    | `DEFAULT` `ALPHA_TEST` | 49/256 | 82/104 | 80.86%    |
| DefaultColorOnlyPBR            | `DEFAULT`              | 65/256 | 86/104 | 74.61%    |
| DefaultColorOnlyPBR            | `ALPHA_TEST`           | 65/256 | 86/104 | 74.61%    |
| DefaultColorOnlyPBR            | `DEFAULT` `ALPHA_TEST` | 65/256 | 86/104 | 74.61%    |
| OITResolve                     |                        | 4/256  | 16/104 | 98.44%    |
| CubemapDownsample              |                        | 7/256  | 22/104 | 97.27%    |
| IrradianceFiltering            |                        | 27/256 | 24/104 | 89.45%    |
| PrecomputeDFG                  |                        | 13/256 | 8/104  | 94.92%    |
| RadianceFiltering              |                        | 23/256 | 24/104 | 91.02%    |
| PostProcess                    |                        | 6/256  | 30/104 | 97.66%    |
| Imgui                          |                        | 14/256 | 18/104 | 94.53%    |
| SMAA_BlendingWeightCalculation |                        | 31/256 | 40/104 | 87.89%    |
| SMAA_EdgeDetection             |                        | 23/256 | 24/104 | 91.02%    |
| SMAA_NeighborhoodBlending      |                        | 17/256 | 26/104 | 93.36%    |

### Compute shaders

| Name             | VGPR   | SGPR   | Occupancy | LDS          | Scratch |
| ---------------- | ------ | ------ | --------- | ------------ | ------- |
| DebugDrawResolve | 18/256 | 32/104 | 92.97%    | 0.00/64.00Kb | 0Kb     |
| Denoise          | 41/256 | 48/104 | 83.98%    | 0.00/64.00Kb | 0Kb     |
| MainPass         | 51/256 | 48/104 | 80.08%    | 0.00/64.00Kb | 0Kb     |
| PrefilterDepth   | 32/256 | 48/104 | 87.50%    | 0.50/64.00Kb | 0Kb     |
| InstancePicking  | 70/256 | 99/104 | 72.66%    | 5.00/64.00Kb | 0Kb     |
| BucketResolve    | 17/256 | 32/104 | 93.36%    | 0.00/64.00Kb | 0Kb     |
| ClusterCulling   | 40/256 | 56/104 | 84.38%    | 0.00/64.00Kb | 0Kb     |
| InstanceCulling  | 40/256 | 74/104 | 84.38%    | 1.00/64.00Kb | 0Kb     |
| LightCulling     | 29/256 | 80/104 | 88.67%    | 0.50/64.00Kb | 0Kb     |
| WorldUpdate      | 19/256 | 24/104 | 92.58%    | 0.00/64.00Kb | 0Kb     |

## RDNA 4.0 (RX 9060)

### Pixel shaders

| Name                           | Permutation            | VGPR   | SGPR   | Occupancy |
| ------------------------------ | ---------------------- | ------ | ------ | --------- |
| BilateralUpsample              |                        | 12/256 | 34/106 | 95.31%    |
| DebugDraw                      |                        | 13/256 | 22/106 | 94.92%    |
| DebugDrawMesh                  |                        | 11/256 | 19/106 | 95.70%    |
| Downsample                     |                        | 5/256  | 22/106 | 98.05%    |
| ComplexSurfacePBR              | `DEFAULT`              | 70/256 | 86/106 | 72.66%    |
| ComplexSurfacePBR              | `ALPHA_TEST`           | 70/256 | 86/106 | 72.66%    |
| ComplexSurfacePBR              | `DEFAULT` `ALPHA_TEST` | 70/256 | 86/106 | 72.66%    |
| DefaultPBR                     | `DEFAULT`              | 56/256 | 86/106 | 78.12%    |
| DefaultPBR                     | `ALPHA_TEST`           | 56/256 | 86/106 | 78.12%    |
| DefaultPBR                     | `DEFAULT` `ALPHA_TEST` | 56/256 | 86/106 | 78.12%    |
| Placeholder                    | `DEFAULT`              | 52/256 | 82/106 | 79.69%    |
| Placeholder                    | `ALPHA_TEST`           | 52/256 | 82/106 | 79.69%    |
| Placeholder                    | `DEFAULT` `ALPHA_TEST` | 52/256 | 82/106 | 79.69%    |
| DefaultColorOnlyPBR            | `DEFAULT`              | 70/256 | 86/106 | 72.66%    |
| DefaultColorOnlyPBR            | `ALPHA_TEST`           | 70/256 | 86/106 | 72.66%    |
| DefaultColorOnlyPBR            | `DEFAULT` `ALPHA_TEST` | 70/256 | 86/106 | 72.66%    |
| OITResolve                     |                        | 4/256  | 12/106 | 98.44%    |
| CubemapDownsample              |                        | 7/256  | 18/106 | 97.27%    |
| IrradianceFiltering            |                        | 27/256 | 24/106 | 89.45%    |
| PrecomputeDFG                  |                        | 13/256 | 8/106  | 94.92%    |
| RadianceFiltering              |                        | 22/256 | 24/106 | 91.41%    |
| PostProcess                    |                        | 6/256  | 34/106 | 97.66%    |
| Imgui                          |                        | 11/256 | 18/106 | 95.70%    |
| SMAA_BlendingWeightCalculation |                        | 30/256 | 40/106 | 88.28%    |
| SMAA_EdgeDetection             |                        | 23/256 | 18/106 | 91.02%    |
| SMAA_NeighborhoodBlending      |                        | 17/256 | 22/106 | 93.36%    |

### Compute shaders

| Name             | VGPR   | SGPR    | Occupancy | LDS         | Scratch |
| ---------------- | ------ | ------- | --------- | ----------- | ------- |
| DebugDrawResolve | 18/256 | 32/106  | 92.97%    | 0.00/0.00Kb | 0Kb     |
| Denoise          | 41/256 | 44/106  | 83.98%    | 0.00/0.00Kb | 0Kb     |
| MainPass         | 52/256 | 48/106  | 79.69%    | 0.00/0.00Kb | 0Kb     |
| PrefilterDepth   | 18/256 | 36/106  | 92.97%    | 0.25/0.50Kb | 0Kb     |
| InstancePicking  | 64/256 | 103/106 | 75.00%    | 4.52/5.00Kb | 0Kb     |
| BucketResolve    | 17/256 | 32/106  | 93.36%    | 0.00/0.00Kb | 0Kb     |
| ClusterCulling   | 38/256 | 60/106  | 85.16%    | 0.00/0.00Kb | 0Kb     |
| InstanceCulling  | 38/256 | 76/106  | 85.16%    | 0.78/1.00Kb | 0Kb     |
| LightCulling     | 29/256 | 78/106  | 88.67%    | 0.03/0.50Kb | 0Kb     |
| WorldUpdate      | 19/256 | 24/106  | 92.58%    | 0.00/0.00Kb | 0Kb     |
