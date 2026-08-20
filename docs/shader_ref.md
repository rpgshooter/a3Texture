# Arma 3 shader reference

Measured from `Dta/bin.pbo` of a retail install, covering the pixel, vertex,
post-process, compute and geometry containers. Counts are from each shader's
DXBC reflection data; stages are read from the shader's own name, which encodes them.

Descriptions marked ✱ are quoted from the official documentation. Descriptions
marked ° were derived by translating the shader and reading what it does; see
the note on method at the end. No shader code is reproduced here.

| container | distinct entry points | compiled blobs | note |
|---|---|---|---|
| `Shaders_5_0_PS.shdc` | 408 | 690 | object, terrain, water, sky, sprite |
| `Shaders_5_0_VS.shdc` | 19 | 3632 | 511 distinct after dedup; 3600 records share `VSShaderPool` |
| `Shaders_5_0_PP.shdc` | 133 | 213 | post-process; holds pixel, vertex and compute stages |
| `Shaders_5_0_CS.shdc` | 5 | 5 | compute |
| `Shaders_5_0_GS.shdc` | 1 | 1 | geometry |

The post-process container is not pixel-only: of its 133 entry points, 108 are
pixel shaders, 18 vertex and 7 compute.

The 4.0 containers mirror the 5.0 ones with the same entry point names.


## object (175)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `DepthOnly` ✱ | 0 | 0 | 6 |  | Special replacement for AlphaOnly for priming non- alpha objects |
| `DepthTerrain` | 0 | 0 | 6 |  | ° . |
| `DetailSpecularAlpha` | 3 | 3 | 373 | S2 _dt, S5 _smdi | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. |
| `DetailSpecularAlphaMacroAS` | 5 | 3 | 388 | S3 _mc + S4 _as, S2 _dt, S5 _smdi | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. |
| `Empty` ✱ | 0 | 0 | 2 |  | empty shader, does not output anything (used only for depth output) |
| `Glass` ✱ | 4 | 4 | 391 |  | glass shader with environmental map |
| `Interpolation` ✱ | 2 | 4 | 182 |  | ° samples 2 material stages (_co, _nohq). Sky dome. |
| `InterpolationAlpha` ✱ | 2 | 0 | 17 |  | ° samples 2 material stages (_co, _nohq). |
| `LODDiag` ✱ | 0 | 1 | 5 |  | shader for lod diagnostics |
| `Multi` ✱ | 16 | 5 | 623 |  | Multi shader |
| `NonTL` ✱ | 1 | 2 | 37 |  | very simple 2D pixel shader |
| `NonTLFlare` ✱ | 4 | 2 | 41 |  | shader to be used for flares |
| `NonTLFlareLight` ✱ | 1 | 0 | 16 |  | shader to be used for flares from dynamic lights (not sun) |
| `NonTLFlareNew` ✱ | 2 | 1 | 21 |  | shader to be used for flares, new HDR version |
| `NonTLFlareNewNoOcclusion` ✱ | 1 | 1 | 14 |  | same as NonTLFlareNew, but without occlusion test |
| `NormalDXTA` ✱ | 2 | 4 | 369 |  | diffuse color modulate, alpha replicate, DXT alpha correction |
| `NormalMap` ✱ | 4 | 3 | 379 | S1 _nohq | normal map shader |
| `NormalMapDetailMacroASSpecularDIMap` ✱ | 7 | 3 | 418 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as, S2 _dt | normal map with detail and ambient shadow and specular map, diffuse is inverse of specular |
| `NormalMapDetailMacroASSpecularMap` ✱ | 7 | 3 | 417 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as, S2 _dt | normal map with detail and ambient shadow and specular map |
| `NormalMapDetailSpecularDIMap` ✱ | 5 | 3 | 402 | S5 _smdi+DI, S1 _nohq, S2 _dt | normal map with detail and specular map, diffuse is inverse of specular |
| `NormalMapDetailSpecularMap` ✱ | 5 | 3 | 401 | S5 _smdi, S1 _nohq, S2 _dt | normal map with detail and specular map |
| `NormalMapMacroAS` ✱ | 5 | 3 | 385 | S1 _nohq, S3 _mc + S4 _as | normal map with ambient shadow texture |
| `NormalMapMacroASSpecularDIMap` ✱ | 6 | 3 | 413 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as | normal map with ambient shadow and specular map, diffuse is inverse of specular |
| `NormalMapMacroASSpecularMap` ✱ | 6 | 3 | 412 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as | normal map with ambient shadow and specular map |
| `NormalMapSpecularDIMap` ✱ | 4 | 3 | 387 | S5 _smdi+DI, S1 _nohq | normal map with specular map, diffuse is inverse of specular |
| `NormalMapSpecularDIMapThermal` | 3 | 4 | 225 | S5 _smdi+DI, S1 _nohq, thermal | ° samples 2 material stages (_co, _nohq); 1 texture at computed coordinates. |
| `NormalMapSpecularMap` ✱ | 4 | 3 | 386 | S5 _smdi, S1 _nohq | normal map with specular map |
| `NormalPiP` ✱ | 2 | 3 | 307 |  | shader for PiP screens |
| `Reflect` | 1 | 0 | 11 |  | ° samples 1 material stage (_co). |
| `ReflectNoShadow` | 1 | 0 | 10 |  | ° samples 1 material stage (_co). |
| `Refract` ✱ | 8 | 5 | 142 |  | shader for refractions _ARMA3_REFRACTION |
| `Road` ✱ | 5 | 5 | 528 |  | road shader |
| `Road2Pass` ✱ | 1 | 3 | 293 |  | road shader - second pass |
| `SRDetailSpecularAlphaMacroAS_Default` | 6 | 3 | 400 | S3 _mc + S4 _as, S2 _dt, S5 _smdi | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRDetailSpecularAlpha_Default` | 4 | 3 | 383 | S2 _dt, S5 _smdi | ° samples 2 material stages (_co, _nohq); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRGlass_Default` | 5 | 4 | 402 |  | ° samples 1 material stage (_co); 2 lighting lookup tables; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Environment reflection. |
| `SRMulti_Default` | 17 | 5 | 636 |  | ° samples 15 textures with model UVs; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SRNormalDXTA_Default` | 3 | 4 | 379 |  | ° samples 1 material stage (_co); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRNormalMapDetailMacroASSpecularDIMap_Default` | 8 | 3 | 430 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as, S2 _dt | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRNormalMapDetailMacroASSpecularMap_Default` | 8 | 3 | 429 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as, S2 _dt | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRNormalMapDetailSpecularDIMap_Default` | 6 | 3 | 412 | S5 _smdi+DI, S1 _nohq, S2 _dt | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRNormalMapDetailSpecularMap_Default` | 6 | 3 | 411 | S5 _smdi, S1 _nohq, S2 _dt | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRNormalMapMacroASSpecularDIMap_Default` | 7 | 3 | 425 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as | ° samples 5 material stages (_co, _nohq, _dt, _mc, _as); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRNormalMapMacroASSpecularMap_Default` | 7 | 3 | 424 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as | ° samples 5 material stages (_co, _nohq, _dt, _mc, _as); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRNormalMapMacroAS_Default` | 6 | 3 | 395 | S1 _nohq, S3 _mc + S4 _as | ° samples 3 material stages (_co, _nohq, _dt); 1 lighting lookup table; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRNormalMapSpecularDIMap_Default` | 5 | 3 | 407 | S5 _smdi+DI, S1 _nohq | ° samples 3 material stages (_co, _nohq, _dt); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRNormalMapSpecularMap_Default` | 5 | 3 | 406 | S5 _smdi, S1 _nohq | ° samples 3 material stages (_co, _nohq, _dt); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRNormalMap_Default` | 5 | 3 | 389 | S1 _nohq | ° samples 2 material stages (_co, _nohq); 1 lighting lookup table; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRNormalPiP_Default` | 2 | 3 | 307 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. |
| `SRRefract_Default` | 8 | 5 | 166 |  | ° samples 3 material stages (_co, _nohq, _dt); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, refraction. |
| `SRRoad2Pass_Default` | 1 | 3 | 293 |  | ° samples 1 material stage (_co). |
| `SRRoad_Default` | 6 | 5 | 538 |  | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SRSkin_Default` | 9 | 6 | 471 |  | ° samples 7 textures with model UVs; 1 lighting lookup table; reads the screen-space SSAO and caustics buffer. Subsurface skin, dynamic point and spot lights. |
| `SRSpecularAlpha_Default` | 3 | 3 | 378 | S5 _smdi | ° samples 1 material stage (_co); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRSpecularNormalMapDiffuseMacroAS_Default` | 7 | 3 | 412 | S1 _nohq, S3 _mc + S4 _as, S5 _smdi | ° samples 5 material stages (_co, _nohq, _dt, _mc, _as); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRSpecularNormalMapDiffuse_Default` | 5 | 3 | 394 | S1 _nohq, S5 _smdi | ° samples 3 material stages (_co, _nohq, _dt); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRSpecularNormalMapSpecularThroughSimple_Default` | 4 | 4 | 396 | S1 _nohq, S5 _smdi, see-through, reduced | ° samples 2 material stages (_co, _nohq); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SRSpecularNormalMapSpecularThrough_Default` | 6 | 4 | 405 | S1 _nohq, S5 _smdi, see-through | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SRSpecularNormalMapThroughSimple_Default` | 4 | 4 | 384 | S1 _nohq, S5 _smdi, see-through, reduced | ° samples 2 material stages (_co, _nohq); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SRSpecularNormalMapThrough_Default` | 6 | 4 | 392 | S1 _nohq, S5 _smdi, see-through | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SRSuperAToC_Default` | 10 | 6 | 574 | alpha-to-coverage | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Environment reflection, dynamic point and spot lights. |
| `SRSuperExt_Default` | 11 | 6 | 588 |  | ° samples 7 textures with model UVs; 2 lighting lookup tables; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Environment reflection, night emissive, dynamic point and spot lights. |
| `SRSuperHairAtoC_Default` | 10 | 7 | 591 |  | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Environment reflection, tree crown alpha, dynamic point and spot lights. |
| `SRSuperHair_Default` | 10 | 7 | 588 |  | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Environment reflection, tree crown alpha, dynamic point and spot lights. |
| `SRSuper_Default` | 10 | 6 | 584 |  | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Environment reflection, dynamic point and spot lights. |
| `SRTerrain15_Default` | 13 | 6 | 1044 |  | ° samples 7 textures with model UVs; 4 textures at computed coordinates; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, dynamic point and spot lights. |
| `SRTerrainNoDetailSNX_Default` | 5 | 6 | 454 | S2 _dt | ° samples 3 textures with model UVs; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SRTerrainNoDetailX_Default` | 4 | 6 | 437 | S2 _dt | ° samples 2 textures with model UVs; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SRTerrainSNX_Default` | 16 | 6 | 1215 |  | ° samples 9 textures with model UVs; 5 textures at computed coordinates; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Terrain layer blending, satellite-normal distance blend, multi-layer terrain, dynamic point and spot lights. |
| `SRTerrainSimple15_Default` | 13 | 6 | 570 | reduced | ° samples 11 textures with model UVs; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, dynamic point and spot lights. |
| `SRTerrainSimpleSNX_Default` | 16 | 6 | 619 | reduced | ° samples 14 textures with model UVs; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Terrain layer blending, satellite-normal distance blend, multi-layer terrain, dynamic point and spot lights. |
| `SRTerrainSimpleX_Default` | 17 | 6 | 613 | reduced | ° samples 15 textures with model UVs; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, dynamic point and spot lights. |
| `SRTerrainX_Default` | 17 | 6 | 1316 |  | ° samples 9 textures with model UVs; 6 textures at computed coordinates; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, dynamic point and spot lights. |
| `SSSMDetailSpecularAlpha` | 4 | 3 | 384 | S2 _dt, S5 _smdi | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. |
| `SSSMDetailSpecularAlphaMacroAS` | 6 | 3 | 401 | S3 _mc + S4 _as, S2 _dt, S5 _smdi | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. |
| `SSSMGlass` | 5 | 4 | 403 |  | ° samples 1 material stage (_co); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection. |
| `SSSMMulti` | 17 | 5 | 637 |  | ° samples 15 textures with model UVs; reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SSSMNormalDXTA` | 3 | 4 | 380 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. |
| `SSSMNormalMap` | 5 | 3 | 390 | S1 _nohq | ° samples 2 material stages (_co, _nohq); 1 lighting lookup table; reads the screen-space SSAO and caustics buffer. |
| `SSSMNormalMapDetailMacroASSpecularDIMap` | 8 | 3 | 431 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as, S2 _dt | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); reads the screen-space SSAO and caustics buffer. |
| `SSSMNormalMapDetailMacroASSpecularMap` | 8 | 3 | 430 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as, S2 _dt | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); reads the screen-space SSAO and caustics buffer. |
| `SSSMNormalMapDetailSpecularDIMap` | 6 | 3 | 413 | S5 _smdi+DI, S1 _nohq, S2 _dt | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. |
| `SSSMNormalMapDetailSpecularMap` | 6 | 3 | 412 | S5 _smdi, S1 _nohq, S2 _dt | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. |
| `SSSMNormalMapMacroAS` | 6 | 3 | 396 | S1 _nohq, S3 _mc + S4 _as | ° samples 3 material stages (_co, _nohq, _dt); 1 lighting lookup table; reads the screen-space SSAO and caustics buffer. |
| `SSSMNormalMapMacroASSpecularDIMap` | 7 | 3 | 426 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as | ° samples 5 material stages (_co, _nohq, _dt, _mc, _as); reads the screen-space SSAO and caustics buffer. |
| `SSSMNormalMapMacroASSpecularMap` | 7 | 3 | 425 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as | ° samples 5 material stages (_co, _nohq, _dt, _mc, _as); reads the screen-space SSAO and caustics buffer. |
| `SSSMNormalMapSpecularDIMap` | 5 | 3 | 408 | S5 _smdi+DI, S1 _nohq | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. |
| `SSSMNormalMapSpecularMap` | 5 | 3 | 407 | S5 _smdi, S1 _nohq | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. |
| `SSSMNormalPiP` | 2 | 3 | 307 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. |
| `SSSMRefract` | 8 | 5 | 166 |  | ° samples 3 material stages (_co, _nohq, _dt); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, refraction. |
| `SSSMRoad` | 6 | 5 | 539 |  | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SSSMRoad2Pass` | 1 | 3 | 293 |  | ° samples 1 material stage (_co). |
| `SSSMSkin` | 9 | 6 | 458 |  | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 1 lighting lookup table; reads the screen-space SSAO and caustics buffer. Subsurface skin, dynamic point and spot lights. |
| `SSSMSpecularAlpha` | 3 | 3 | 379 | S5 _smdi | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. |
| `SSSMSpecularNormalMapDiffuse` | 5 | 3 | 395 | S1 _nohq, S5 _smdi | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. |
| `SSSMSpecularNormalMapDiffuseMacroAS` | 7 | 3 | 413 | S1 _nohq, S3 _mc + S4 _as, S5 _smdi | ° samples 5 material stages (_co, _nohq, _dt, _mc, _as); reads the screen-space SSAO and caustics buffer. |
| `SSSMSpecularNormalMapSpecularThrough` | 6 | 4 | 406 | S1 _nohq, S5 _smdi, see-through | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SSSMSpecularNormalMapSpecularThroughSimple` | 4 | 4 | 397 | S1 _nohq, S5 _smdi, see-through, reduced | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SSSMSpecularNormalMapThrough` | 6 | 4 | 393 | S1 _nohq, S5 _smdi, see-through | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SSSMSpecularNormalMapThroughSimple` | 4 | 4 | 385 | S1 _nohq, S5 _smdi, see-through, reduced | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SSSMSuper` | 10 | 6 | 585 |  | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, dynamic point and spot lights. |
| `SSSMSuperAToC` | 10 | 6 | 575 | alpha-to-coverage | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, dynamic point and spot lights. |
| `SSSMSuperExt` | 11 | 6 | 589 |  | ° samples 7 textures with model UVs; 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, night emissive, dynamic point and spot lights. |
| `SSSMSuperHair` | 10 | 7 | 589 |  | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, tree crown alpha, dynamic point and spot lights. |
| `SSSMSuperHairAtoC` | 10 | 7 | 592 |  | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, tree crown alpha, dynamic point and spot lights. |
| `SSSMTerrain15` | 13 | 6 | 1045 |  | ° samples 7 textures with model UVs; 4 textures at computed coordinates; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, dynamic point and spot lights. |
| `SSSMTerrainNoDetailSNX` | 5 | 6 | 455 | S2 _dt | ° samples 3 textures with model UVs; reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SSSMTerrainNoDetailX` | 4 | 6 | 438 | S2 _dt | ° samples 2 textures with model UVs; reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SSSMTerrainSNX` | 16 | 6 | 1216 |  | ° samples 9 textures with model UVs; 5 textures at computed coordinates; reads the screen-space SSAO and caustics buffer. Terrain layer blending, satellite-normal distance blend, multi-layer terrain, dynamic point and spot lights. |
| `SSSMTerrainSimple15` | 13 | 6 | 571 | reduced | ° samples 11 textures with model UVs; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, dynamic point and spot lights. |
| `SSSMTerrainSimpleSNX` | 16 | 6 | 630 | reduced | ° samples 14 textures with model UVs; reads the screen-space SSAO and caustics buffer. Terrain layer blending, satellite-normal distance blend, multi-layer terrain, dynamic point and spot lights. |
| `SSSMTerrainSimpleX` | 17 | 6 | 614 | reduced | ° samples 15 textures with model UVs; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, dynamic point and spot lights. |
| `SSSMTerrainX` | 17 | 6 | 1317 |  | ° samples 9 textures with model UVs; 6 textures at computed coordinates; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, dynamic point and spot lights. |
| `ShadowBufferAlpha` | 1 | 0 | 10 |  | ° samples 1 material stage (_co). |
| `Skin` ✱ | 8 | 6 | 445 |  | Human skin - derived from Super shader |
| `SpecularAlpha` | 2 | 3 | 368 | S5 _smdi | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. |
| `SpecularAlphaThermal` | 3 | 4 | 383 | S5 _smdi, thermal | ° samples 3 textures with model UVs. |
| `SpecularNormalMapDiffuse` | 4 | 3 | 384 | S1 _nohq, S5 _smdi | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. |
| `SpecularNormalMapDiffuseMacroAS` | 6 | 3 | 400 | S1 _nohq, S3 _mc + S4 _as, S5 _smdi | ° samples 5 material stages (_co, _nohq, _dt, _mc, _as); reads the screen-space SSAO and caustics buffer. |
| `SpecularNormalMapSpecularThrough` | 5 | 4 | 397 | S1 _nohq, S5 _smdi, see-through | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SpecularNormalMapSpecularThroughSimple` | 3 | 4 | 378 | S1 _nohq, S5 _smdi, see-through, reduced | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SpecularNormalMapThrough` | 5 | 4 | 384 | S1 _nohq, S5 _smdi, see-through | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SpecularNormalMapThroughSimple` | 3 | 4 | 376 | S1 _nohq, S5 _smdi, see-through, reduced | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `Super` ✱ | 9 | 6 | 572 |  | Super shader |
| `SuperAToC` ✱ | 9 | 6 | 572 | alpha-to-coverage | Super shader AToC variant |
| `SuperAToCThermal` | 7 | 4 | 418 | thermal, alpha-to-coverage | ° samples 7 textures with model UVs. |
| `SuperExt` ✱ | 10 | 6 | 576 |  | skyscraper & building, intended as super shader light version |
| `SuperHair` ✱ | 9 | 7 | 578 |  | super shader for hair rendering |
| `SuperHairAtoC` ✱ | 9 | 7 | 579 |  | super shader for hair rendering, atoc version |
| `SuperThermal` | 7 | 4 | 421 | thermal | ° samples 7 textures with model UVs. |
| `ThermalCrater1` | 2 | 6 | 201 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater10` | 2 | 6 | 255 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater11` | 2 | 6 | 261 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater12` | 2 | 6 | 267 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater13` | 2 | 6 | 273 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater14` | 2 | 6 | 279 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater2` | 2 | 6 | 207 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater3` | 2 | 6 | 213 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater4` | 2 | 6 | 219 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater5` | 2 | 6 | 225 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater6` | 2 | 6 | 231 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater7` | 2 | 6 | 237 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater8` | 2 | 6 | 243 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalCrater9` | 2 | 6 | 249 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Environment reflection, crater temperature, crater blending. |
| `ThermalDetailSpecularAlpha` | 3 | 4 | 226 | S2 _dt, S5 _smdi, thermal | ° samples 2 material stages (_co, _nohq); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalDetailSpecularAlphaMacroAS` | 4 | 4 | 229 | S3 _mc + S4 _as, S2 _dt, S5 _smdi, thermal | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalGlass` | 2 | 4 | 221 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalInterpolation` | 0 | 0 | 4 | thermal | ° . |
| `ThermalInterpolationAlpha` | 3 | 1 | 42 | thermal | ° samples 2 material stages (_co, _nohq); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalMulti` | 16 | 4 | 327 | thermal | ° samples 15 textures with model UVs; 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalNormalDXTA` | 2 | 5 | 223 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalNormalMap` | 3 | 4 | 223 | S1 _nohq, thermal | ° samples 2 material stages (_co, _nohq); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalNormalMapDetailMacroASSpecularDIMap` | 5 | 4 | 240 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as, S2 _dt, thermal | ° samples 4 material stages (_co, _nohq, _dt, _mc); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalNormalMapDetailMacroASSpecularMap` | 5 | 4 | 240 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as, S2 _dt, thermal | ° samples 4 material stages (_co, _nohq, _dt, _mc); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalNormalMapDetailSpecularDIMap` | 4 | 4 | 235 | S5 _smdi+DI, S1 _nohq, S2 _dt, thermal | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalNormalMapDetailSpecularMap` | 4 | 4 | 235 | S5 _smdi, S1 _nohq, S2 _dt, thermal | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalNormalMapMacroAS` | 4 | 4 | 230 | S1 _nohq, S3 _mc + S4 _as, thermal | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalNormalMapMacroASSpecularDIMap` | 4 | 4 | 235 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as, thermal | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalNormalMapMacroASSpecularMap` | 4 | 4 | 235 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as, thermal | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalNormalMapSpecularMap` | 3 | 4 | 228 | S5 _smdi, S1 _nohq, thermal | ° samples 2 material stages (_co, _nohq); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalRefract` | 5 | 4 | 115 | thermal | ° samples 2 material stages (_co, _nohq); 1 texture at computed coordinates; reads the screen-space SSAO and caustics buffer. Refraction, thermal imaging range. |
| `ThermalRoad` | 4 | 4 | 229 | thermal | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalRoad2Pass` | 2 | 4 | 197 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalSkin` | 3 | 4 | 373 | thermal | ° samples 3 textures with model UVs. |
| `ThermalSpecularNormalMapDiffuse` | 4 | 4 | 235 | S1 _nohq, S5 _smdi, thermal | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalSpecularNormalMapDiffuseMacroAS` | 5 | 4 | 240 | S1 _nohq, S3 _mc + S4 _as, S5 _smdi, thermal | ° samples 4 material stages (_co, _nohq, _dt, _mc); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalSpecularNormalMapSpecularThrough` | 4 | 5 | 235 | S1 _nohq, S5 _smdi, thermal, see-through | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range, tree crown alpha. |
| `ThermalSpecularNormalMapSpecularThroughSimple` | 3 | 5 | 230 | S1 _nohq, S5 _smdi, thermal, see-through, reduced | ° samples 2 material stages (_co, _nohq); 1 texture at computed coordinates. Thermal imaging range, tree crown alpha. |
| `ThermalSpecularNormalMapThrough` | 4 | 5 | 233 | S1 _nohq, S5 _smdi, thermal, see-through | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range, tree crown alpha. |
| `ThermalSpecularNormalMapThroughSimple` | 3 | 5 | 227 | S1 _nohq, S5 _smdi, thermal, see-through, reduced | ° samples 2 material stages (_co, _nohq); 1 texture at computed coordinates. Thermal imaging range, tree crown alpha. |
| `ThermalSuperExt` | 5 | 4 | 245 | thermal | ° samples 4 material stages (_co, _nohq, _dt, _mc); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalSuperHair` | 5 | 5 | 251 | thermal | ° samples 4 material stages (_co, _nohq, _dt, _mc); 1 texture at computed coordinates. Thermal imaging range, tree crown alpha. |
| `ThermalSuperHairAtoC` | 5 | 5 | 252 | thermal | ° samples 4 material stages (_co, _nohq, _dt, _mc); 1 texture at computed coordinates. Thermal imaging range, tree crown alpha. |
| `White` ✱ | 0 | 0 | 4 |  | ° . |
| `WhiteAlpha` ✱ | 1 | 0 | 9 |  | ° samples 1 material stage (_co). |
## foliage (92)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `AlphaOnlyTree` | 2 | 2 | 52 |  | ° samples 2 material stages (_co, _nohq). Tree crown alpha. |
| `AlphaOnlyTreeAToC` | 1 | 2 | 52 | alpha-to-coverage | ° samples 1 material stage (_co). Tree crown alpha. |
| `AlphaOnlyTreeAdv` | 1 | 2 | 48 |  | ° samples 1 material stage (_co). Tree crown alpha. |
| `AlphaOnlyTreeAdvAToC` | 1 | 2 | 52 | alpha-to-coverage | ° samples 1 material stage (_co). Tree crown alpha. |
| `DEBUGSHDGrass` | 2 | 3 | 96 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDGrassAToC` | 2 | 3 | 94 | alpha-to-coverage | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDSpecularNormalMapGrass` | 3 | 3 | 81 | S1 _nohq, S5 _smdi | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDTerrainGrass15` | 3 | 4 | 74 |  | ° samples 2 textures with model UVs; reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDTerrainGrassX` | 3 | 4 | 74 |  | ° samples 2 textures with model UVs; reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDTree` | 4 | 4 | 126 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Tree crown alpha, debug visualisation. |
| `DEBUGSHDTreeAToC` | 4 | 4 | 126 | alpha-to-coverage | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Tree crown alpha, debug visualisation. |
| `DEBUGSHDTreeAdv` | 4 | 4 | 172 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha, debug visualisation. |
| `DEBUGSHDTreeAdvAToC` | 4 | 4 | 173 | alpha-to-coverage | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha, debug visualisation. |
| `DEBUGSHDTreeAdvSimple` | 4 | 4 | 129 | reduced | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha, debug visualisation. |
| `DEBUGSHDTreeAdvSimpleAToC` | 4 | 4 | 130 | alpha-to-coverage, reduced | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha, debug visualisation. |
| `DEBUGSHDTreeAdvTrans` | 4 | 4 | 171 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha, debug visualisation. |
| `DEBUGSHDTreeAdvTransAToC` | 4 | 4 | 172 | alpha-to-coverage | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha, debug visualisation. |
| `DEBUGSHDTreeAdvTrunk` | 4 | 4 | 160 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, debug visualisation. |
| `DEBUGSHDTreeAdvTrunkSimple` | 4 | 4 | 122 | reduced | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, debug visualisation. |
| `DEBUGSHDTreePRT` | 4 | 5 | 104 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Precomputed radiance transfer, tree crown alpha, debug visualisation. |
| `DEBUGSHDTreeSN` | 4 | 4 | 125 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Tree crown alpha, debug visualisation. |
| `DEBUGSHDTreeSimple` | 4 | 4 | 111 | reduced | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Tree crown alpha, debug visualisation. |
| `Grass` ✱ | 2 | 3 | 366 |  | grass shader - alpha discretized |
| `GrassAToC` ✱ | 2 | 3 | 364 | alpha-to-coverage | grass with alpha to coverage |
| `SRGrassAToC_Default` | 3 | 3 | 374 | alpha-to-coverage | ° samples 1 material stage (_co); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRGrass_Default` | 3 | 3 | 376 |  | ° samples 1 material stage (_co); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRSpecularNormalMapGrass_Default` | 4 | 3 | 361 | S1 _nohq, S5 _smdi | ° samples 2 material stages (_co, _nohq); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRTerrainGrass15_Default` | 4 | 6 | 440 |  | ° samples 2 textures with model UVs; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SRTerrainGrassX_Default` | 4 | 6 | 440 |  | ° samples 2 textures with model UVs; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SRTreeAToC_Default` | 5 | 4 | 398 | alpha-to-coverage | ° samples 3 material stages (_co, _nohq, _dt); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SRTreeAdvAToC_Default` | 4 | 4 | 319 | alpha-to-coverage | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SRTreeAdvSimpleAToC_Default` | 4 | 4 | 277 | alpha-to-coverage, reduced | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SRTreeAdvSimple_Default` | 4 | 4 | 276 | reduced | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SRTreeAdvTransAToC_Default` | 4 | 4 | 319 | alpha-to-coverage | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SRTreeAdvTrans_Default` | 4 | 4 | 318 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SRTreeAdvTrunkSimple_Default` | 4 | 4 | 247 | reduced | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree. |
| `SRTreeAdvTrunk_Default` | 4 | 4 | 285 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree. |
| `SRTreeAdv_Default` | 4 | 4 | 318 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SRTreePRT_Default` | 5 | 5 | 372 |  | ° samples 3 material stages (_co, _nohq, _dt); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Precomputed radiance transfer, tree crown alpha. |
| `SRTreeSN_Default` | 5 | 4 | 405 |  | ° samples 3 material stages (_co, _nohq, _dt); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SRTreeSimple_Default` | 5 | 4 | 389 | reduced | ° samples 3 material stages (_co, _nohq, _dt); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SRTree_Default` | 5 | 4 | 406 |  | ° samples 3 material stages (_co, _nohq, _dt); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SSSMGrass` | 3 | 3 | 377 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. |
| `SSSMGrassAToC` | 3 | 3 | 375 | alpha-to-coverage | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. |
| `SSSMSpecularNormalMapGrass` | 4 | 3 | 362 | S1 _nohq, S5 _smdi | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. |
| `SSSMTerrainGrass15` | 4 | 6 | 441 |  | ° samples 2 textures with model UVs; reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SSSMTerrainGrassX` | 4 | 6 | 431 |  | ° samples 2 textures with model UVs; reads the screen-space SSAO and caustics buffer. Dynamic point and spot lights. |
| `SSSMTree` | 5 | 4 | 407 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SSSMTreeAToC` | 5 | 4 | 409 | alpha-to-coverage | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SSSMTreeAdv` | 5 | 4 | 328 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SSSMTreeAdvAToC` | 5 | 4 | 331 | alpha-to-coverage | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SSSMTreeAdvSimple` | 5 | 4 | 286 | reduced | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SSSMTreeAdvSimpleAToC` | 5 | 4 | 289 | alpha-to-coverage, reduced | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SSSMTreeAdvTrans` | 5 | 4 | 328 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SSSMTreeAdvTransAToC` | 5 | 4 | 331 | alpha-to-coverage | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree, tree crown alpha. |
| `SSSMTreeAdvTrunk` | 5 | 4 | 297 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree. |
| `SSSMTreeAdvTrunkSimple` | 5 | 4 | 259 | reduced | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Advanced tree. |
| `SSSMTreePRT` | 5 | 5 | 383 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Precomputed radiance transfer, tree crown alpha. |
| `SSSMTreeSN` | 5 | 4 | 406 |  | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SSSMTreeSimple` | 5 | 4 | 390 | reduced | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Tree crown alpha. |
| `SpecularNormalMapGrass` | 3 | 3 | 351 | S1 _nohq, S5 _smdi | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. |
| `ThermalGrass` | 2 | 4 | 221 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalGrassAToC` | 2 | 4 | 219 | thermal, alpha-to-coverage | ° samples 1 material stage (_co); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalSpecularNormalMapGrass` | 2 | 4 | 212 | S1 _nohq, S5 _smdi, thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalTerrainGrass15` | 3 | 5 | 222 | thermal | ° samples 2 textures with model UVs; 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalTerrainGrassX` | 3 | 5 | 222 | thermal | ° samples 2 textures with model UVs; 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalTree` | 4 | 5 | 239 | thermal | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range, tree crown alpha. |
| `ThermalTreeAToC` | 4 | 5 | 239 | thermal, alpha-to-coverage | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range, tree crown alpha. |
| `ThermalTreeAdv` | 3 | 4 | 363 | thermal | ° samples 2 textures with model UVs; 1 texture at computed coordinates. Tree crown alpha. |
| `ThermalTreeAdvAToC` | 3 | 4 | 364 | thermal, alpha-to-coverage | ° samples 2 textures with model UVs; 1 texture at computed coordinates. Tree crown alpha. |
| `ThermalTreeAdvSimple` | 3 | 4 | 363 | thermal, reduced | ° samples 2 textures with model UVs; 1 texture at computed coordinates. Tree crown alpha. |
| `ThermalTreeAdvSimpleAToC` | 3 | 4 | 364 | thermal, alpha-to-coverage, reduced | ° samples 2 textures with model UVs; 1 texture at computed coordinates. Tree crown alpha. |
| `ThermalTreeAdvTrans` | 3 | 4 | 363 | thermal | ° samples 2 textures with model UVs; 1 texture at computed coordinates. Tree crown alpha. |
| `ThermalTreeAdvTransAToC` | 3 | 4 | 364 | thermal, alpha-to-coverage | ° samples 2 textures with model UVs; 1 texture at computed coordinates. Tree crown alpha. |
| `ThermalTreeAdvTrunk` | 3 | 3 | 356 | thermal | ° samples 2 textures with model UVs; 1 texture at computed coordinates. |
| `ThermalTreeAdvTrunkSimple` | 3 | 3 | 356 | thermal, reduced | ° samples 2 textures with model UVs; 1 texture at computed coordinates. |
| `ThermalTreePRT` | 3 | 5 | 226 | thermal | ° samples 2 textures with model UVs; 1 texture at computed coordinates. Thermal imaging range, tree crown alpha. |
| `ThermalTreeSN` | 4 | 5 | 238 | thermal | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range, tree crown alpha. |
| `ThermalTreeSimple` | 4 | 5 | 236 | thermal, reduced | ° samples 3 material stages (_co, _nohq, _dt); 1 texture at computed coordinates. Thermal imaging range, tree crown alpha. |
| `Tree` ✱ | 4 | 4 | 388 |  | Tree shader |
| `TreeAToC` ✱ | 4 | 4 | 398 | alpha-to-coverage | tree with alpha to coverage |
| `TreeAdv` ✱ | 4 | 4 | 318 |  | advanced tree crown shader |
| `TreeAdvAToC` ✱ | 4 | 4 | 319 | alpha-to-coverage | advanced tree crown shader |
| `TreeAdvSimple` ✱ | 4 | 4 | 276 | reduced | advanced tree crown shader |
| `TreeAdvSimpleAToC` ✱ | 4 | 4 | 277 | alpha-to-coverage, reduced | advanced tree crown shader |
| `TreeAdvTrans` ✱ | 4 | 4 | 312 |  | same as TreeAdv, but there is translucency map in alpha channel of MCA texture ( instead o |
| `TreeAdvTransAToC` ✱ | 4 | 4 | 319 | alpha-to-coverage | same as TreeAdv, but there is translucency map in alpha channel of MCA texture ( instead o |
| `TreeAdvTrunk` ✱ | 4 | 4 | 285 |  | advanced tree shader |
| `TreeAdvTrunkSimple` ✱ | 4 | 4 | 247 | reduced | advanced tree shader |
| `TreePRT` ✱ | 4 | 5 | 374 |  | Tree shader - very cheap shader with PRT |
| `TreeSN` ✱ | 4 | 4 | 397 |  | Tree shader width simple noise |
| `TreeSimple` ✱ | 4 | 4 | 371 | reduced | Tree shader - simpler version of Tree |

## terrain (33)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `Crater1` ✱ | 2 | 5 | 329 |  | Crater rendering - X craters |
| `Crater10` ✱ | 2 | 5 | 365 |  | Crater rendering - X craters |
| `Crater11` ✱ | 2 | 5 | 369 |  | Crater rendering - X craters |
| `Crater12` ✱ | 2 | 5 | 373 |  | Crater rendering - X craters |
| `Crater13` ✱ | 2 | 5 | 377 |  | Crater rendering - X craters |
| `Crater14` ✱ | 2 | 5 | 381 |  | Crater rendering - X craters |
| `Crater2` ✱ | 2 | 5 | 333 |  | Crater rendering - X craters |
| `Crater3` ✱ | 2 | 5 | 337 |  | Crater rendering - X craters |
| `Crater4` ✱ | 2 | 5 | 341 |  | Crater rendering - X craters |
| `Crater5` ✱ | 2 | 5 | 345 |  | Crater rendering - X craters |
| `Crater6` ✱ | 2 | 5 | 349 |  | Crater rendering - X craters |
| `Crater7` ✱ | 2 | 5 | 353 |  | Crater rendering - X craters |
| `Crater8` ✱ | 2 | 5 | 357 |  | Crater rendering - X craters |
| `Crater9` ✱ | 2 | 5 | 361 |  | Crater rendering - X craters |
| `Terrain15` ✱ | 12 | 6 | 1034 |  | terrain - X layers |
| `Terrain15Thermal` | 12 | 5 | 806 | thermal | ° samples 7 textures with model UVs; 5 textures at computed coordinates. Terrain layer blending, multi-layer terrain. |
| `TerrainGrass15` ✱ | 3 | 6 | 420 |  | terrain grass - X layers |
| `TerrainGrassAlphaX` | 0 | 0 | 6 |  | ° . |
| `TerrainGrassX` ✱ | 3 | 6 | 420 |  | terrain grass - general number of layers |
| `TerrainNoDetailSNX` ✱ | 4 | 6 | 444 | S2 _dt | terrainSNX without detail map |
| `TerrainNoDetailSNXThermal` | 4 | 5 | 225 | S2 _dt, thermal | ° samples 3 textures with model UVs; 1 texture at computed coordinates. |
| `TerrainNoDetailX` ✱ | 3 | 6 | 427 | S2 _dt | terrainX without detail map |
| `TerrainNoDetailXThermal` | 3 | 5 | 214 | S2 _dt, thermal | ° samples 2 textures with model UVs; 1 texture at computed coordinates. |
| `TerrainSNX` ✱ | 15 | 6 | 1205 |  | terrain - general number of layers + satellite normal map |
| `TerrainSNXThermal` | 15 | 5 | 957 | thermal | ° samples 9 textures with model UVs; 6 textures at computed coordinates. Terrain layer blending, satellite-normal distance blend, multi-layer terrain. |
| `TerrainSimple15` ✱ | 12 | 6 | 560 | reduced | terrainSimple - X layers |
| `TerrainSimple15Thermal` | 12 | 5 | 325 | thermal, reduced | ° samples 11 textures with model UVs; 1 texture at computed coordinates. Terrain layer blending, multi-layer terrain. |
| `TerrainSimpleSNX` ✱ | 15 | 6 | 619 | reduced | terrainSNX without parallax mapping |
| `TerrainSimpleSNXThermal` | 15 | 5 | 376 | thermal, reduced | ° samples 14 textures with model UVs; 1 texture at computed coordinates. Terrain layer blending, satellite-normal distance blend, multi-layer terrain. |
| `TerrainSimpleX` ✱ | 16 | 6 | 603 | reduced | terrainSimple - general number of layers |
| `TerrainSimpleXThermal` | 16 | 5 | 368 | thermal, reduced | ° samples 15 textures with model UVs; 1 texture at computed coordinates. Terrain layer blending, multi-layer terrain. |
| `TerrainX` ✱ | 16 | 6 | 1306 |  | terrain - general number of layers |
| `TerrainXThermal` | 16 | 5 | 1067 | thermal | ° samples 9 textures with model UVs; 7 textures at computed coordinates. Terrain layer blending, multi-layer terrain. |
## water (17)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `CalmWater` ✱ | 9 | 2 | 174 |  | calm water surface |
| `CalmWaterTi` | 1 | 0 | 19 |  | ° 1 texture at computed coordinates. |
| `Caustics` ✱ | 1 | 2 | 75 |  | shader for caustics effect |
| `CausticsSSSM` | 2 | 2 | 82 |  | ° samples 1 textures with model UVs; reads the screen-space SSAO and caustics buffer. |
| `DEBUGSHDWater` | 10 | 5 | 1748 |  | ° samples 3 textures with model UVs; 5 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, wave colouring, calm water, screen-space water reflection, debug visualisation. |
| `SRCalmWater_Default` | 11 | 3 | 211 |  | ° samples 4 material stages (_co, _nohq, _dt, _mc); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Calm water. |
| `SRWater_Default` | 10 | 7 | 1938 |  | ° samples 3 textures with model UVs; 5 lighting lookup tables; reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. Environment reflection, wave colouring, calm water, screen-space water reflection, dynamic point and spot lights. |
| `SSSMCalmWater` | 11 | 3 | 209 |  | ° samples 4 material stages (_co, _nohq, _dt, _mc); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Calm water. |
| `SSSMWater` | 10 | 7 | 1938 |  | ° samples 3 textures with model UVs; 5 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, wave colouring, calm water, screen-space water reflection, dynamic point and spot lights. |
| `Shore` ✱ | 12 | 7 | 2099 |  | shore shader |
| `ShoreFoam` ✱ | 1 | 4 | 303 |  | shore shader for the foam on the top of the shore |
| `ShoreWet` ✱ | 0 | 3 | 293 |  | shore shader for the wet part |
| `ThermalWater` | 1 | 3 | 164 | thermal | ° 1 texture at computed coordinates. |
| `UnderwaterOcclusion` ✱ | 0 | 2 | 23 |  | Shader used for underwater occlusion object |
| `UnderwaterOcclusionThermal` | 0 | 0 | 6 | thermal | ° . |
| `Water` ✱ | 9 | 7 | 1923 |  | sea water |
| `WaterSimple` ✱ | 2 | 3 | 52 | reduced | small water |

## sky (18)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `Cloud` ✱ | 1 | 2 | 19 |  | Shader used for clouds |
| `CloudThermal` | 1 | 1 | 13 | thermal | ° samples 1 material stage (_co). |
| `DEBUGSHDVolCloud` | 3 | 3 | 59 |  | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDVolCloudSimple` | 2 | 3 | 55 | reduced | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `Horizon` ✱ | 2 | 3 | 190 |  | Shader used for the horizon |
| `HorizonThermal` | 1 | 0 | 24 | thermal | ° 1 texture at computed coordinates. |
| `SRVolCloudSimple_Default` | 3 | 3 | 615 | reduced | ° samples 1 material stage (_co); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRVolCloud_Default` | 4 | 3 | 619 |  | ° samples 2 material stages (_co, _nohq); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SSSMVolCloud` | 4 | 3 | 620 |  | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. |
| `SSSMVolCloudSimple` | 3 | 3 | 616 | reduced | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. |
| `SimulWeatherClouds` ✱ | 7 | 2 | 97 |  | SimulWeather clouds |
| `SimulWeatherCloudsCPU` ✱ | 3 | 2 | 65 |  | SimulWeather clouds with CPU distance fading |
| `SimulWeatherCloudsWithLightning` ✱ | 8 | 2 | 107 |  | SimulWeather clouds with lightning |
| `SimulWeatherCloudsWithLightningCPU` ✱ | 4 | 2 | 75 |  | SimulWeather clouds with lightning and CPU distance fading |
| `ThermalVolCloud` | 3 | 4 | 213 | thermal | ° samples 2 material stages (_co, _nohq); 1 texture at computed coordinates. Thermal imaging range. |
| `ThermalVolCloudSimple` | 2 | 4 | 194 | thermal, reduced | ° samples 1 material stage (_co); 1 texture at computed coordinates. Thermal imaging range. |
| `VolCloud` ✱ | 3 | 3 | 607 |  | Shader used for volumetric cloud - it uses SoftParticle approach |
| `VolCloudSimple` ✱ | 2 | 3 | 605 | reduced | Shader used for volumetric cloud - no SoftParticle approach |

## sprite (25)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `DEBUGSHDSprite` | 3 | 3 | 66 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDSpriteExtTi` | 3 | 3 | 66 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDSpriteRefract` | 4 | 3 | 65 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Refraction, debug visualisation. |
| `DEBUGSHDSpriteRefractSimple` | 3 | 3 | 55 | reduced | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Refraction, debug visualisation. |
| `DEBUGSHDSpriteSimple` | 2 | 3 | 57 | reduced | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `SRSpriteExtTi_Default` | 3 | 3 | 76 |  | ° samples 1 material stage (_co); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SRSpriteRefractSimple_Default` | 3 | 3 | 45 | reduced | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Refraction. |
| `SRSpriteRefract_Default` | 4 | 3 | 55 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Refraction. |
| `SRSpriteSimple_Default` | 2 | 3 | 66 | reduced | ° samples 1 material stage (_co); reads a hardware shadow map. |
| `SRSprite_Default` | 3 | 3 | 76 |  | ° samples 1 material stage (_co); reads a hardware shadow map; reads the screen-space SSAO and caustics buffer. |
| `SSSMSprite` | 3 | 3 | 76 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. |
| `SSSMSpriteExtTi` | 3 | 3 | 76 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. |
| `SSSMSpriteRefract` | 4 | 3 | 55 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Refraction. |
| `SSSMSpriteRefractSimple` | 3 | 3 | 45 | reduced | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Refraction. |
| `SSSMSpriteSimple` | 2 | 3 | 68 | reduced | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. |
| `Sprite` ✱ | 2 | 3 | 66 |  | Shader used for sprite rendering - it uses SoftParticle approach |
| `SpriteExtTi` ✱ | 2 | 3 | 66 |  | Sprite used for vehicles covering |
| `SpriteRefract` ✱ | 4 | 3 | 55 |  | _ARMA3_REFRACTION_SPRITES - Shader used for sprite rendering with refraction - it uses Sof |
| `SpriteRefractSimple` ✱ | 3 | 3 | 45 | reduced | _ARMA3_REFRACTION_SPRITES - Shader used for sprite rendering with refraction- no SoftParti |
| `SpriteSimple` ✱ | 1 | 3 | 56 | reduced | Shader used for sprite rendering - no SoftParticle approach |
| `ThermalSprite` | 3 | 4 | 69 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates; reads the screen-space SSAO and caustics buffer. Thermal imaging range. |
| `ThermalSpriteExtTi` | 3 | 4 | 77 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates; reads the screen-space SSAO and caustics buffer. Thermal imaging range. |
| `ThermalSpriteRefract` | 4 | 4 | 66 | thermal | ° samples 1 material stage (_co); 1 texture at computed coordinates; reads the screen-space SSAO and caustics buffer. Refraction, thermal imaging range. |
| `ThermalSpriteRefractSimple` | 3 | 4 | 56 | thermal, reduced | ° samples 1 material stage (_co); 1 texture at computed coordinates; reads the screen-space SSAO and caustics buffer. Refraction, thermal imaging range. |
| `ThermalSpriteSimple` | 2 | 4 | 59 | thermal, reduced | ° samples 1 material stage (_co); 1 texture at computed coordinates. Thermal imaging range. |
## ui (8)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `AlphaOnly` | 1 | 1 | 43 |  | ° samples 1 material stage (_co). |
| `AlphaOnlyMod` | 1 | 1 | 45 |  | ° samples 1 material stage (_co). |
| `AlphaOnlyModAToC` | 1 | 1 | 45 | alpha-to-coverage | ° samples 1 material stage (_co). |
| `AlphaOnlyNoAlpha` | 0 | 1 | 31 |  | ° . |
| `Collimator` ✱ | 1 | 3 | 291 |  | special shader for collimator |
| `CollimatorThermal` | 0 | 0 | 4 | thermal | ° . |
| `Point` ✱ | 0 | 3 | 297 |  | Shader used for point lights |
| `PointThermal` | 1 | 1 | 13 | thermal | ° samples 1 material stage (_co). |
## debug (40)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `DEBUGSHDDetailSpecularAlpha` | 3 | 3 | 99 | S2 _dt, S5 _smdi | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDDetailSpecularAlphaMacroAS` | 5 | 3 | 111 | S3 _mc + S4 _as, S2 _dt, S5 _smdi | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDGlass` | 4 | 4 | 117 |  | ° samples 1 material stage (_co); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, debug visualisation. |
| `DEBUGSHDMulti` | 16 | 3 | 228 |  | ° samples 15 textures with model UVs; reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalDXTA` | 2 | 4 | 97 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalMap` | 4 | 3 | 105 | S1 _nohq | ° samples 2 material stages (_co, _nohq); 1 lighting lookup table; reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalMapDetailMacroASSpecularDIMap` | 7 | 3 | 142 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as, S2 _dt | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalMapDetailMacroASSpecularMap` | 7 | 3 | 141 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as, S2 _dt | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalMapDetailSpecularDIMap` | 5 | 3 | 128 | S5 _smdi+DI, S1 _nohq, S2 _dt | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalMapDetailSpecularMap` | 5 | 3 | 127 | S5 _smdi, S1 _nohq, S2 _dt | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalMapMacroAS` | 5 | 3 | 111 | S1 _nohq, S3 _mc + S4 _as | ° samples 3 material stages (_co, _nohq, _dt); 1 lighting lookup table; reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalMapMacroASSpecularDIMap` | 6 | 3 | 137 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as | ° samples 5 material stages (_co, _nohq, _dt, _mc, _as); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalMapMacroASSpecularMap` | 6 | 3 | 136 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as | ° samples 5 material stages (_co, _nohq, _dt, _mc, _as); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalMapSpecularDIMap` | 4 | 3 | 123 | S5 _smdi+DI, S1 _nohq | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalMapSpecularMap` | 4 | 3 | 122 | S5 _smdi, S1 _nohq | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDNormalPiP` | 2 | 3 | 44 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDRefract` | 8 | 5 | 167 |  | ° samples 3 material stages (_co, _nohq, _dt); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, refraction, debug visualisation. |
| `DEBUGSHDRoad` | 5 | 3 | 134 |  | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDRoad2Pass` | 2 | 3 | 52 |  | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDSkin` | 8 | 4 | 194 |  | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 1 lighting lookup table; reads the screen-space SSAO and caustics buffer. Subsurface skin, debug visualisation. |
| `DEBUGSHDSpecularAlpha` | 2 | 3 | 94 | S5 _smdi | ° samples 1 material stage (_co); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDSpecularNormalMapDiffuse` | 4 | 3 | 110 | S1 _nohq, S5 _smdi | ° samples 3 material stages (_co, _nohq, _dt); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDSpecularNormalMapDiffuseMacroAS` | 6 | 3 | 124 | S1 _nohq, S3 _mc + S4 _as, S5 _smdi | ° samples 5 material stages (_co, _nohq, _dt, _mc, _as); reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDSpecularNormalMapSpecularThrough` | 5 | 4 | 125 | S1 _nohq, S5 _smdi, see-through | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. Tree crown alpha, debug visualisation. |
| `DEBUGSHDSpecularNormalMapSpecularThroughSimple` | 3 | 4 | 116 | S1 _nohq, S5 _smdi, see-through, reduced | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. Tree crown alpha, debug visualisation. |
| `DEBUGSHDSpecularNormalMapThrough` | 5 | 4 | 114 | S1 _nohq, S5 _smdi, see-through | ° samples 4 material stages (_co, _nohq, _dt, _mc); reads the screen-space SSAO and caustics buffer. Tree crown alpha, debug visualisation. |
| `DEBUGSHDSpecularNormalMapThroughSimple` | 3 | 4 | 106 | S1 _nohq, S5 _smdi, see-through, reduced | ° samples 2 material stages (_co, _nohq); reads the screen-space SSAO and caustics buffer. Tree crown alpha, debug visualisation. |
| `DEBUGSHDSuper` | 9 | 4 | 183 |  | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, debug visualisation. |
| `DEBUGSHDSuperAToC` | 9 | 4 | 183 | alpha-to-coverage | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, debug visualisation. |
| `DEBUGSHDSuperExt` | 10 | 4 | 187 |  | ° samples 7 textures with model UVs; 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, night emissive, debug visualisation. |
| `DEBUGSHDSuperHair` | 9 | 5 | 189 |  | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, tree crown alpha, debug visualisation. |
| `DEBUGSHDSuperHairAtoC` | 9 | 5 | 190 |  | ° samples 6 material stages (_co, _nohq, _dt, _mc, _as, _smdi); 2 lighting lookup tables; reads the screen-space SSAO and caustics buffer. Environment reflection, tree crown alpha, debug visualisation. |
| `DEBUGSHDTerrain15` | 12 | 4 | 668 |  | ° samples 7 textures with model UVs; 4 textures at computed coordinates; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, debug visualisation. |
| `DEBUGSHDTerrainNoDetailSNX` | 4 | 4 | 88 | S2 _dt | ° samples 3 textures with model UVs; reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDTerrainNoDetailX` | 3 | 4 | 71 | S2 _dt | ° samples 2 textures with model UVs; reads the screen-space SSAO and caustics buffer. Debug visualisation. |
| `DEBUGSHDTerrainSNX` | 15 | 4 | 826 |  | ° samples 9 textures with model UVs; 5 textures at computed coordinates; reads the screen-space SSAO and caustics buffer. Terrain layer blending, satellite-normal distance blend, multi-layer terrain, debug visualisation. |
| `DEBUGSHDTerrainSimple15` | 12 | 4 | 187 | reduced | ° samples 11 textures with model UVs; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, debug visualisation. |
| `DEBUGSHDTerrainSimpleSNX` | 15 | 4 | 245 | reduced | ° samples 14 textures with model UVs; reads the screen-space SSAO and caustics buffer. Terrain layer blending, satellite-normal distance blend, multi-layer terrain, debug visualisation. |
| `DEBUGSHDTerrainSimpleX` | 16 | 4 | 230 | reduced | ° samples 15 textures with model UVs; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, debug visualisation. |
| `DEBUGSHDTerrainX` | 16 | 4 | 929 |  | ° samples 9 textures with model UVs; 6 textures at computed coordinates; reads the screen-space SSAO and caustics buffer. Terrain layer blending, multi-layer terrain, debug visualisation. |
## Vertex shaders

The container holds 3632 records under only 19 entry point names, 3600 of them
called `VSShaderPool`. Deduplicating by content shows the repetition is largely
storage: the 3632 records are **511 distinct shaders**, of which 486 are pool
variants and 18 are named specialists. All 511 translate cleanly.

### The uber-shader pool

The 486 pool variants reduce to **38 distinct combinations** of input signature,
constant buffers and textures. These are the axes that vary, each close to an
even split, which is what a compiled permutation matrix looks like:

| axis | on | off | evidence |
|---|--:|--:|---|
| skinned | 242 | 244 | `BLENDWEIGHT` and `BLENDINDICES` in the input signature |
| instanced | 246 | 240 | `instTransform0-2` attributes |
| bone matrix texture | 242 | 244 | binds `animMatrixTexture` |
| dynamic lights | 214 | 272 | binds `VSCB_Lights1` and `VSCB_Lights2` |
| skinning-instancing block | 118 | 368 | binds `VSCB_SkinningInstancing` |
| tree | 82 | 404 | binds `VSCB_Tree` |

Input signatures within the pool number exactly four, and they are the skinning
and instancing axes crossed: plain, skinned, instanced, and both, at 122, 118,
122 and 124 variants.

**Skinning is texture-based.** Binding `animMatrixTexture` matches carrying
`BLENDWEIGHT` exactly, 242 to 242 with no exceptions either way, and the same
pairing holds in the named `VSShadowVolume` family. Bone matrices are fetched
from a texture rather than uploaded as constants, which is what lets one shader
serve any bone count.

Below those axes the variants still differ in code, since 38 reflection
combinations cover 486 blobs. Those remaining differences are compile-time
branches that reflection does not expose; translated size ranges from 108 to 692
lines of GLSL against a median of 403.

### Named vertex shaders

Everything the pool does not cover. These are addressed by name and do not permute.

| entry point | variants | behaviour |
|---|--:|---|
| `VSCalmWater` | 1 | ° 2 UV sets; 2 tangent streams; emits 9 varyings. Per-object transform. |
| `VSNonTL` | 1 | ° 2 UV sets; emits 3 varyings. |
| `VSPoint` | 1 | ° 1 UV set; emits 10 varyings. Per-object transform. |
| `VSRoad` | 1 | ° 2 UV sets; 2 tangent streams; emits 11 varyings. Per-object transform. |
| `VSShadowVolume` | 1 | ° 2 input attributes; emits position only. Per-object transform. |
| `VSShadowVolumeInstanced` | 1 | ° instanced with per-instance transform and colour; emits position only. Per-object transform. |
| `VSShadowVolumeSkinned` | 1 | ° skinned, 3 weight sets fetched against a bone matrix texture; emits position only. Combined skinning and instancing block, per-object transform. |
| `VSShadowVolumeSkinnedInstanced` | 1 | ° skinned, 3 weight sets fetched against a bone matrix texture; instanced with per-instance transform and colour; emits position only. Per-object transform. |
| `VSShore` | 1 | ° 1 UV set; 2 tangent streams; emits 9 varyings. Per-object transform. |
| `VSSimulWeatherClouds` | 3 | ° 4 UV sets; emits 10 varyings; samples 2 textures. Per-object transform, volumetric cloud density. |
| `VSSprite` | 1 | ° 1 UV set; emits 9 varyings. Dynamic point and spot lights, per-object transform. |
| `VSSpriteOnSurface` | 1 | ° 1 UV set; emits 9 varyings. Dynamic point and spot lights, per-object transform. |
| `VSStar` | 1 | ° 1 UV set; emits 10 varyings. Per-object transform. |
| `VSTerrain` | 4 | ° 2 UV sets; 2 tangent streams; emits 12 varyings. Per-object transform. |
| `VSTerrainGrass` | 3 | ° 2 UV sets; 2 tangent streams; emits 7 varyings. Per-object transform. |
| `VSUnderwaterOcclusion` | 1 | ° 2 UV sets; 2 tangent streams; emits 3 varyings. Per-object transform. |
| `VSVolCloud` | 1 | ° 1 UV set; emits 9 varyings. Combined skinning and instancing block, per-object transform. |
| `VSWater` | 1 | ° 1 UV set; emits 9 varyings. Per-object transform. |

### The pool's 38 combinations

Each row is one distinct combination of input signature, constant buffers and
textures. Several compiled blobs share a row, differing only in code paths that
reflection does not expose, so the GLSL size range is given instead.

| blobs | skinned | instanced | lights | tree | skin+inst block | GLSL lines |
|--:|---|---|---|---|---|---|
| 42 | yes | yes | yes | — | — | 573–645 |
| 40 | — | — | yes | — | — | 337–406 |
| 40 | — | yes | yes | — | — | 352–424 |
| 40 | yes | — | yes | — | yes | 610–685 |
| 34 | yes | — | — | — | yes | 502–579 |
| 32 | — | — | — | — | — | 231–300 |
| 32 | — | yes | — | — | — | 239–318 |
| 32 | yes | yes | — | — | — | 451–539 |
| 12 | — | — | yes | yes | — | 383–407 |
| 12 | — | yes | yes | yes | — | 404–431 |
| 12 | yes | yes | yes | yes | — | 625–652 |
| 10 | — | — | — | — | — | 124–203 |
| 10 | — | — | — | yes | — | 281–300 |
| 10 | yes | — | — | — | yes | 396–478 |
| 10 | yes | yes | — | yes | — | 523–545 |
| 10 | — | yes | — | yes | — | 302–324 |
| 8 | yes | — | — | yes | yes | 568–585 |
| 8 | — | yes | — | — | — | 118–163 |
| 8 | yes | — | yes | yes | yes | 674–692 |
| 8 | yes | — | — | — | yes | 503–528 |
| 8 | yes | yes | — | — | — | 448–487 |
| 8 | yes | yes | — | — | — | 330–384 |
| 8 | — | — | — | — | — | 230–254 |
| 8 | — | yes | — | — | — | 236–266 |
| 6 | — | — | — | — | — | 108–146 |
| 6 | yes | yes | — | — | — | 320–369 |
| 6 | yes | — | — | — | yes | 380–420 |
| 6 | — | yes | — | — | — | 108–148 |
| 2 | yes | yes | — | — | — | 410–418 |
| 2 | yes | — | — | — | yes | 385–390 |
| 2 | — | — | — | — | — | 112–117 |
| 2 | — | — | yes | — | — | 301–309 |
| 2 | — | yes | — | — | — | 198–206 |
| 2 | yes | yes | yes | — | — | 516–524 |
| 2 | yes | — | yes | — | yes | 576–584 |
| 2 | — | yes | — | — | — | 112–117 |
| 2 | yes | yes | — | — | — | 325–330 |
| 2 | — | yes | yes | — | — | 304–312 |

## Post-process

The screen-space effects. This container holds 133 entry points across three
stages: 108 pixel, 18 vertex and 7 compute. None is reachable
from a material; they run as engine passes. Descriptions are derived by the same
translation method used for the object shaders.

### Anti-aliasing (15)

| entry point | stage | behaviour |
|---|---|---|
| `A3_SMAABlendingWeightCalculationPS` | pixel | ° reads 3 input targets; 13 texture reads; iterative. Anti-aliasing. |
| `A3_SMAABlendingWeightCalculationVS` | vertex | ° vertex stage, no texture input. |
| `A3_SMAAColorEdgeDetectionPS` | pixel | ° reads 2 input targets; 10 texture reads. |
| `A3_SMAAEdgeDetectionVS` | vertex | ° vertex stage, no texture input. |
| `A3_SMAANeighborhoodBlendingPS` | pixel | ° reads 2 input targets; 6 texture reads. Anti-aliasing. |
| `A3_SMAANeighborhoodBlendingVS` | vertex | ° vertex stage, no texture input. |
| `DbgDisplayEdgesPS` | pixel | ° reads 1 input target. |
| `Edges0PS` | pixel | ° reads 1 input target. |
| `Edges1PS` | pixel | ° reads 1 input target. |
| `EdgesCombinePS` | pixel | ° pixel stage, no texture input. |
| `PsFxaa3_11` | pixel | ° reads 1 input target; 12 texture reads. Anti-aliasing. |
| `PsFxaa3_11Luma` | pixel | ° reads 1 input target. |
| `VSPostProcessCMAA` | vertex | ° vertex stage, no texture input. |
| `VSPostProcessCustomEdge4T` | vertex | ° vertex stage, no texture input. |
| `VsFxaa3_11` | vertex | ° vertex stage, no texture input. |

### Ambient occlusion (15)

| entry point | stage | behaviour |
|---|---|---|
| `CSHDAO` | compute | ° compute stage, no texture input. |
| `PSHDAO` | pixel | ° reads 1 input target; 9 texture reads. |
| `PSPostProcessSSAO` | pixel | ° reads 2 input targets; 37 texture reads. Screen-space ambient occlusion. |
| `PSPostProcessSSAOBlurHorz` | pixel | ° reads 1 input target; 7 texture reads. Screen-space ambient occlusion. |
| `PSPostProcessSSAOBlurHorzDepth` | pixel | ° reads 2 input targets; 15 texture reads. Screen-space ambient occlusion. |
| `PSPostProcessSSAOBlurVert` | pixel | ° reads 1 input target; 7 texture reads. Screen-space ambient occlusion. |
| `PSPostProcessSSAOBlurVertDepth` | pixel | ° reads 2 input targets; 15 texture reads. Screen-space ambient occlusion. |
| `PSPostProcessSSAODebug` | pixel | ° reads 1 input target. |
| `PSPostProcessSSAODown` | pixel | ° reads 1 input target; 4 texture reads. |
| `PSPostProcessSSAOFinal` | pixel | ° reads 1 input target. |
| `PSSSAO` | pixel | ° reads 1 input target; 33 texture reads. |
| `PSSSAOBlurHoriz` | pixel | ° reads 2 input targets; 10 texture reads. |
| `PSSSAOBlurVert` | pixel | ° reads 2 input targets; 10 texture reads. |
| `VSPostProcessSSAO` | vertex | ° vertex stage, no texture input. |
| `VSPostProcessSSAODown` | vertex | ° vertex stage, no texture input. |

### Shadow filtering (5)

| entry point | stage | behaviour |
|---|---|---|
| `PSPostProcessSSSMHigh` | pixel | ° reads 1 input target; 17 texture reads; samples a shadow map. |
| `PSPostProcessSSSMLow` | pixel | ° reads 1 input target; samples a shadow map. |
| `PSPostProcessSSSMNormal` | pixel | ° reads 1 input target; 5 texture reads; samples a shadow map. |
| `PSPostProcessSSSMStencil` | pixel | ° pixel stage, no texture input. |
| `PSPostProcessSSSMVeryHigh` | pixel | ° reads 1 input target; 17 texture reads; samples a shadow map. |

### God rays (6)

| entry point | stage | behaviour |
|---|---|---|
| `PSGodRaysApplyClouds` | pixel | ° reads 2 input targets. |
| `PSGodRaysBlur` | pixel | ° reads 1 input target; iterative. God rays. |
| `PSGodRaysCompose` | pixel | ° reads 1 input target. God rays. |
| `PSGodRaysDownscaleH` | pixel | ° reads 1 input target; 5 texture reads. God rays. |
| `PSGodRaysDownscaleV` | pixel | ° reads 1 input target; 5 texture reads. God rays. |
| `PSGodRaysLightSource` | pixel | ° God rays. |

### Bloom (7)

| entry point | stage | behaviour |
|---|---|---|
| `PSPostProcessBloomCombine` | pixel | ° reads 2 input targets. Bloom. |
| `PSPostProcessBloomDownsample2` | pixel | ° reads 2 input targets; 5 texture reads. Bloom. |
| `PSPostProcessBloomDownsample4` | pixel | ° reads 1 input target; iterative. |
| `PSPostProcessGlowNewBloomCompose` | pixel | ° reads 1 input target. |
| `PSPostProcessGlowNewBloomInitDownsample` | pixel | ° reads 1 input target. Bloom. |
| `PSPostProcessGlowNewBloomInitDownsample4x` | pixel | ° reads 4 input targets. Bloom. |
| `PSPostProcessGlowNewBloomKawase` | pixel | ° reads 1 input target; 4 texture reads. Bloom. |

### Depth of field (2)

| entry point | stage | behaviour |
|---|---|---|
| `PSPostProcessDOF` | pixel | ° reads 3 input targets; 6 texture reads; iterative. |
| `PSPostProcessDistanceDOF` | pixel | ° reads 3 input targets; 5 texture reads; iterative. |

### Exposure and tone (18)

| entry point | stage | behaviour |
|---|---|---|
| `CSCalculateHistogramLum` | compute | ° reads 1 input target; iterative. |
| `CSCalculateHistogramThermal` | compute | ° reads 1 input target; iterative. |
| `CSComputeCDFFromHistogram` | compute | ° iterative. |
| `CSMergeHistogram` | compute | ° iterative. |
| `PSPostProcessAssumedLuminance` | pixel | ° reads 2 input targets. |
| `PSPostProcessAssumedLuminanceDirect` | pixel | ° reads 1 input target. |
| `PSPostProcessColorsSimple` | pixel | ° reads 1 input target. |
| `PSPostProcessDownSampleAvgLuminance` | pixel | ° reads 4 input targets. |
| `PSPostProcessDownSampleLuminance` | pixel | ° reads 4 input targets. |
| `PSPostProcessDownSampleMaxAvgMinLuminance` | pixel | ° reads 4 input targets. |
| `PSPostProcessGlowNewLuminanceAvg2x` | pixel | ° reads 1 input target. |
| `PSPostProcessGlowNewLuminanceAvg4x` | pixel | ° reads 4 input targets. |
| `PSPostProcessGlowNewLuminanceInit` | pixel | ° reads 4 input targets. Bloom. |
| `PSPostProcessHDRColorLimit` | pixel | ° reads 1 input target. Bloom. |
| `PSPostProcessHDRColorLimitNVG` | pixel | ° reads 1 input target. Bloom. |
| `PsPpColors` | pixel | ° reads 1 input target. |
| `PsPpColorsRadial` | pixel | ° reads 2 input targets. Radial colour grading. |
| `VsPpColors` | vertex | ° vertex stage, no texture input. |

### Blur and distortion (19)

| entry point | stage | behaviour |
|---|---|---|
| `CSFilterX` | compute | ° reads 2 input targets; 18 texture reads. |
| `CSFilterY` | compute | ° reads 2 input targets; 18 texture reads. |
| `PSFilterX` | pixel | ° reads 2 input targets; 10 texture reads. |
| `PSFilterY` | pixel | ° reads 2 input targets; 10 texture reads. |
| `PSPostProcessGaussBlur` | pixel | ° reads 1 input target; iterative. |
| `PSPostProcessGaussianBlurH` | pixel | ° reads 1 input target; 5 texture reads. |
| `PSPostProcessGaussianBlurV` | pixel | ° reads 1 input target; 5 texture reads. |
| `PSPostProcessRescaleBicubic` | pixel | ° reads 2 input targets; 6 texture reads. |
| `PSPostProcessSharpen` | pixel | ° reads 1 input target; 17 texture reads. Anti-aliasing. |
| `PsPpDynamicBlur` | pixel | ° reads 1 input target; 11 texture reads. Dynamic blur. |
| `PsPpDynamicBlurFinal` | pixel | ° reads 2 input targets; 5 texture reads. |
| `PsPpRadialBlur` | pixel | ° reads 1 input target; iterative. Radial blur. |
| `PsPpRotBlur` | pixel | ° reads 2 input targets; 30 texture reads. |
| `PsPpWetDistort` | pixel | ° reads 2 input targets; 6 texture reads. |
| `VSPostProcessRescaleBicubic` | vertex | ° vertex stage, no texture input. |
| `VsPpDynamicBlur` | vertex | ° vertex stage, no texture input. |
| `VsPpDynamicBlurFinal` | vertex | ° vertex stage, no texture input. |
| `VsPpRotBlur` | vertex | ° Rotational blur. |
| `VsPpWetDistort` | vertex | ° Wet lens distortion. |

### Weather and water (8)

| entry point | stage | behaviour |
|---|---|---|
| `PSPostProcessCaustics` | pixel | ° reads 2 input targets. Caustics. |
| `PSPostProcessRain3D` | pixel | ° reads 2 input targets. |
| `PSPostProcessRainParticles` | pixel | ° reads 3 input targets. |
| `PSRainOccluderGrowH` | pixel | ° reads 1 input target. |
| `PSRainOccluderGrowV` | pixel | ° reads 1 input target. |
| `PSRainRefractAdjust` | pixel | ° reads 1 input target. |
| `VSPostProcessRain3D` | vertex | ° vertex stage, no texture input. |
| `VSPostProcessRainParticles` | vertex | ° Rain. |

### Vision modes (7)

| entry point | stage | behaviour |
|---|---|---|
| `PSPostProcessGlowNewFinalNightFilmic` | pixel | ° reads 2 input targets. Bloom, eye adaptation. |
| `PSPostProcessGlowNewFinalNightNone` | pixel | ° reads 2 input targets. Bloom, eye adaptation. |
| `PSPostProcessGlowNewFinalNightReinhard` | pixel | ° reads 2 input targets. Bloom, eye adaptation. |
| `PSPostProcessGlowNight` | pixel | ° reads 4 input targets. Night vision, eye adaptation. |
| `PSPostProcessThermal` | pixel | ° reads 2 input targets. Thermal imaging. |
| `PSPostProcessThermalPresent` | pixel | ° reads 2 input targets. Blur. |
| `PsPpClrInversion` | pixel | ° reads 1 input target. Colour inversion. |

### Copy and resolve (7)

| entry point | stage | behaviour |
|---|---|---|
| `PSCopy` | pixel | ° reads 1 input target. |
| `PSPostProcessCopy` | pixel | ° reads 1 input target. |
| `PSPostProcessCopyAlphaOne` | pixel | ° reads 1 input target. |
| `PSPostProcessCopyResolve` | pixel | ° reads 1 input target; iterative. |
| `PSPostProcessCopyResolveAlphaOne` | pixel | ° reads 1 input target; iterative. |
| `PSPostProcessDepthResolveFirstSample` | pixel | ° reads 1 input target. |
| `PSPostProcessDepthResolveMax` | pixel | ° reads 1 input target; iterative. |

### Debug and query (3)

| entry point | stage | behaviour |
|---|---|---|
| `PSOcclusionQueryDebug` | pixel | ° pixel stage, no texture input. |
| `PSPostProcessDiagDepthBuffer` | pixel | ° reads 1 input target. Bloom. |
| `VSOcclusionQuery` | vertex | ° vertex stage, no texture input. |

### Other (21)

| entry point | stage | behaviour |
|---|---|---|
| `PSPostProcessFilmGrainColor` | pixel | ° reads 2 input targets. |
| `PSPostProcessFilmGrainMono` | pixel | ° reads 2 input targets. |
| `PSPostProcessFisheye` | pixel | ° reads 1 input target. Anti-aliasing. |
| `PSPostProcessGlow` | pixel | ° reads 4 input targets. Bloom. |
| `PSPostProcessGlowNewFinalFilmic` | pixel | ° reads 2 input targets. Bloom, eye adaptation. |
| `PSPostProcessGlowNewFinalNVG` | pixel | ° reads 2 input targets. Bloom, eye adaptation. |
| `PSPostProcessGlowNewFinalNone` | pixel | ° reads 2 input targets. Bloom, eye adaptation. |
| `PSPostProcessGlowNewFinalReinhard` | pixel | ° reads 2 input targets. Bloom, eye adaptation. |
| `PSPostProcessGlowNewQuery` | pixel | ° pixel stage, no texture input. |
| `PSPostProcessNVG` | pixel | ° reads 3 input targets. Eye adaptation. |
| `PSPostProcessQuery` | pixel | ° reads 2 input targets. |
| `PSPostProcessSimulWeather` | pixel | ° reads 1 input target. |
| `PSPostProcessSimulWeatherDepthDecimate` | pixel | ° reads 1 input target. |
| `PSPostProcessSimulWeatherDepthDecimateHorizontal` | pixel | ° reads 1 input target. |
| `PSPostProcessSimulWeatherDepthDecimateVertical` | pixel | ° reads 1 input target. |
| `PSPrepareSSDepthMap` | pixel | ° reads 1 input target. Screen-space water reflection. |
| `PSPrepareSSDepthMapFromDepthInfo` | pixel | ° reads 1 input target. |
| `PSPrepareSSSceneTexture` | pixel | ° reads 1 input target. |
| `ProcessAndApplyPS` | pixel | ° reads 3 input targets; 11 texture reads; iterative. |
| `PsPpChromAber` | pixel | ° reads 1 input target; 3 texture reads. Chromatic aberration. |
| `VSPostProcess` | vertex | ° vertex stage, no texture input. |

## Documented names versus compiled names

The documentation lists 153 pixel shader IDs. 97 appear in
`Shaders_5_0_PS.shdc` under exactly that name. Another 7 are compiled under a
name with a lighting-model prefix attached, so a search for the documented name
alone finds nothing even though the shader exists.

| documented ID | compiled as |
|---|---|
| `NormalMapDiffuse` | `SpecularNormalMapDiffuse` |
| `NormalMapDiffuseMacroAS` | `SpecularNormalMapDiffuseMacroAS` |
| `NormalMapGrass` | `SpecularNormalMapGrass` |
| `NormalMapSpecularThrough` | `SpecularNormalMapSpecularThrough` |
| `NormalMapSpecularThroughSimple` | `SpecularNormalMapSpecularThroughSimple` |
| `NormalMapThrough` | `SpecularNormalMapThrough` |
| `NormalMapThroughSimple` | `SpecularNormalMapThroughSimple` |

### No compiled shader (49)

Each of these was searched for directly in the raw bytes of all nine shader
containers, as a whole token, under every prefix the engine uses. None occurs.
See the validation note below for the method.

42 of the 49 are the `Terrain`, `TerrainGrass` and `TerrainSimple` runs from 1
to 14. For each of those three families only the `15` variant is compiled,
alongside a variable-layer `X` form (`PSTerrainX`, `PSTerrainSimpleX`,
`PSTerrainGrassX`) and `SNX` and `NoDetail` variants. So a layer count below
fifteen is served by the `X` shader or by the `15` one, never by a shader named
for that count.

The remaining seven divide in two. `AlphaNoShadow`, `AlphaShadow`, `Dummy0` and
`NormalMapThroughLowEnd` do not occur anywhere in any container, even as a
substring, and appear to be retired. `Normal`, `Detail` and `DetailMacroAS`
occur only inside longer names such as `PSNormalMap` and
`PSNormalMapDetailMacroASSpecularMap`; there is no standalone shader by those
names, so the documented bare IDs have nothing behind them.

| documented ID | description |
|---|---|
| `AlphaNoShadow` | shadow alpha (no shadow write |
| `AlphaShadow` | shadow alpha write |
| `Detail` | detail texturing |
| `DetailMacroAS` | detail with ambient shadow texture |
| `Dummy0` |  |
| `Normal` | diffuse color modulate, alpha replicate |
| `NormalMapThroughLowEnd` | substitute shader for NormalMapThrough shaders for low-end settings |
| `Terrain1` | terrain - X layers |
| `Terrain10` | terrain - X layers |
| `Terrain11` | terrain - X layers |
| `Terrain12` | terrain - X layers |
| `Terrain13` | terrain - X layers |
| `Terrain14` | terrain - X layers |
| `Terrain2` | terrain - X layers |
| `Terrain3` | terrain - X layers |
| `Terrain4` | terrain - X layers |
| `Terrain5` | terrain - X layers |
| `Terrain6` | terrain - X layers |
| `Terrain7` | terrain - X layers |
| `Terrain8` | terrain - X layers |
| `Terrain9` | terrain - X layers |
| `TerrainGrass1` | terrain grass - X layers |
| `TerrainGrass10` | terrain grass - X layers |
| `TerrainGrass11` | terrain grass - X layers |
| `TerrainGrass12` | terrain grass - X layers |
| `TerrainGrass13` | terrain grass - X layers |
| `TerrainGrass14` | terrain grass - X layers |
| `TerrainGrass2` | terrain grass - X layers |
| `TerrainGrass3` | terrain grass - X layers |
| `TerrainGrass4` | terrain grass - X layers |
| `TerrainGrass5` | terrain grass - X layers |
| `TerrainGrass6` | terrain grass - X layers |
| `TerrainGrass7` | terrain grass - X layers |
| `TerrainGrass8` | terrain grass - X layers |
| `TerrainGrass9` | terrain grass - X layers |
| `TerrainSimple1` | terrainSimple - X layers |
| `TerrainSimple10` | terrainSimple - X layers |
| `TerrainSimple11` | terrainSimple - X layers |
| `TerrainSimple12` | terrainSimple - X layers |
| `TerrainSimple13` | terrainSimple - X layers |
| `TerrainSimple14` | terrainSimple - X layers |
| `TerrainSimple2` | terrainSimple - X layers |
| `TerrainSimple3` | terrainSimple - X layers |
| `TerrainSimple4` | terrainSimple - X layers |
| `TerrainSimple5` | terrainSimple - X layers |
| `TerrainSimple6` | terrainSimple - X layers |
| `TerrainSimple7` | terrainSimple - X layers |
| `TerrainSimple8` | terrainSimple - X layers |
| `TerrainSimple9` | terrainSimple - X layers |



## Documented vertex shaders versus compiled ones

The documentation lists 44 vertex shader IDs. Checked against the containers
with the same prefix-aware, whole-token method used for the pixel shaders, with
positive and negative controls, they fall into three groups.

| outcome | count |
|---|--:|
| has a real `VS` entry point | 14 |
| exists only as a pixel shader | 12 |
| no entry point anywhere | 18 |

Fewer than a third of the documented vertex shader IDs name an actual vertex
shader.

### Real vertex shaders (14)

`CalmWater`, `Point`, `Road`, `ShadowVolume`, `Shore`, `SimulWeatherClouds`, `Sprite`, `SpriteOnSurface`, `Star`, `Terrain`, `TerrainGrass`, `UnderwaterOcclusion`, `VolCloud`, `Water`

### Listed as vertex shaders but compiled only as pixel shaders (12)

These names exist, so a search finds them and the ID looks valid, but there is
no `VS` entry point behind any of them. They are pixel shaders that the vertex
list repeats.

| ID | compiled as |
|---|---|
| `Grass` | `PSGrass` |
| `Multi` | `PSMulti` |
| `NormalMap` | `PSNormalMap` |
| `Refract` | `PSRefract` |
| `SimulWeatherCloudsCPU` | `PSSimulWeatherCloudsCPU` |
| `Skin` | `PSSkin` |
| `Super` | `PSSuper` |
| `Tree` | `PSTree` |
| `TreeAdv` | `PSTreeAdv` |
| `TreeAdvTrunk` | `PSTreeAdvTrunk` |
| `TreePRT` | `PSTreePRT` |
| `WaterSimple` | `PSWaterSimple` |

### No entry point at all (18)

| ID | described as |
|---|---|
| `Basic` | N/A |
| `BasicAS` | ambient shadow |
| `BasicFade` | basic with face fading (based on the angle with camera direction |
| `Dummy1` |  |
| `Dummy2` |  |
| `Dummy3` |  |
| `NormalMapAS` | normal map with ambient shadow |
| `NormalMapDiffuse` | normal map + detail map |
| `NormalMapDiffuseAS` | diffuse normal map with ambient shadow Glass /*glass shader*/ \\ |
| `NormalMapSpecularThrough` | normal map with specular - tree shader |
| `NormalMapSpecularThroughNoFade` | normal map with specular - tree shader - without face fading |
| `NormalMapThrough` | normal map - tree shader |
| `NormalMapThroughNoFade` | normal map - tree shader - without face fading |
| `SimulWeatherCloudsGS` | simul weather clouds with geom shader |
| `TreeAdvModNormals` | advanced tree crown shader with modified vertex normals |
| `TreeAdvNoFade` | advanced tree crown shader - no face fading |
| `TreeNoFade` | Tree shader - cheap shader designed for trees and bushes - without face fading |
| `TreePRTNoFade` | Tree shader - very cheap shader designed for trees and bushes - without face fading |

The `Through` and `NoFade` tree shaders are the notable absence here. The
documentation describes a family of face-fading and non-fading tree variants,
and none of them exists as a vertex shader. The fading behaviour lives in the
pixel shaders, where `SpecularNormalMapThrough` and its relatives are compiled.

### Undocumented compiled vertex shaders

The omission runs the other way too. Five compiled vertex entry points appear
nowhere in the documentation:

`NonTL`, `ShaderPool`, `ShadowVolumeInstanced`, `ShadowVolumeSkinned`,
`ShadowVolumeSkinnedInstanced`

`ShaderPool` is the significant one. It is the entry point behind 3600 of the
3632 vertex records, so the shader that performs nearly all vertex work in the
engine is the one the documentation never mentions. The documented vertex list
describes the specialists and omits the general case.

## How the absence claims were validated

Absence is easy to claim and easy to get wrong, so the negative results above
were tested rather than inferred.

Entry points are stored in the containers with a stage prefix: the shader the
documentation calls `Super` is stored as `PSSuper`, and `ShaderPool` as
`VSShaderPool`. A search for the documented name alone finds nothing for almost
every shader in the set, which produces a convincing and completely wrong list
of missing shaders. The first run of this check did exactly that and reported
all 49 as absent for the wrong reason.

The method that produced the results above:

- all nine `.shdc` containers extracted and searched as raw bytes, rather than
  searching a name list derived from an earlier pass
- matches required to be whole tokens, so `Terrain1` does not match inside
  `Terrain15`
- every candidate tried under each stage prefix and bare
- a positive control of seven names known to exist (`Super`, `NormalMap`,
  `Terrain15`, `Glass`, `Water`, `SpecularNormalMapThrough`, `ShaderPool`), all
  of which were found
- a negative control of invented names (`ZZZNotAShader`, `Terrain99`), none of
  which were found

As a completeness check on the reference as a whole, the pixel shader container
yields 556 distinct `PS*` tokens. 148 are constant buffer and sampler names,
leaving 408 entry points, and all 408 appear in this document. The list is the
container's contents, not a sample of them.


## How the descriptions were derived

93 of the 408 pixel shaders have an official description. The rest, and all of
the post-process and vertex shaders, were described by translating each shader
out of its compiled form and reading the result, rather than by guessing from
the name.

Each blob was converted from DXBC to SPIR-V and then to GLSL. Everything
converted without error: 408 pixel, 133 post-process, and 511 vertex shaders
after deduplication, 1052 in total. The translated code was then analysed for facts that survive
translation intact:

- which texture units are read, and how each is addressed. A unit sampled at
  interpolated model UVs is a material texture; one sampled at coordinates built
  from a dot product is a lighting lookup table; one read with `texelFetch` at
  scaled `gl_FragCoord` is a screen-space buffer; one read through a shadow
  sampler is a shadow map.
- which named constants the reflection data marks as actually used, which
  identifies features such as refraction, terrain layer blending or thermal
  range.
- whether the shader discards fragments, loops over lights, or writes more than
  one render target.

Reading the code rather than the name corrects things name inference gets wrong.
In `NormalMap`, texture unit 3 is bound and used, and a slot-order reading would
call it the `_mc` macro texture. It is sampled at dot-product coordinates, so it
is a lighting lookup table, and the shader has only two material textures.

Two facts fell out with no exceptions across the corpus, and the descriptions
rely on them:

- **Unit 16 is the screen-space SSAO and caustics buffer.** 264 shaders read it,
  264 shaders use `PSC_SSAOCausticsScale`, and they are the same 264. It is
  always read with `texelFetch` at `gl_FragCoord` scaled by the inverse viewport
  size.
- **`SR..._Default` and `SSSM` are shadow-receiving variants.** Exactly 52
  shaders sample a real depth-comparison shadow map, and all 52 carry one of
  those two prefixes. Both add `PSC_Shadow_Factor_ZHalf` over the base shader,
  and `SSSM` additionally uses the viewport size, consistent with a screen-space
  filtering step. `DEBUGSHD` variants instead add `PSC_ShaderDebugMode` and drop
  the fog, water and dynamic-light constants entirely.

What the method does not recover: the intent behind a shader, the meaning of an
uncommented magic constant, and the exact lighting model. A description here
says what the shader reads and what it computes with, not why.