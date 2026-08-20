# Arma 3 pixel shader reference

Measured from `Dta/bin.pbo` of a retail install. Counts are from each shader's
DXBC reflection data; stages are read from the shader's own name, which encodes them.

Descriptions marked ✱ are quoted from the official documentation. Descriptions
marked ° were derived by translating the shader and reading what it does; see
the note on method at the end. No shader code is reproduced here.

| container | distinct entry points | compiled blobs | note |
|---|---|---|---|
| `Shaders_5_0_PS.shdc` | 408 | 690 | object, terrain, water, sky, sprite |
| `Shaders_5_0_VS.shdc` | 19 | 3632 | most share `VSShaderPool`; names cannot disambiguate |
| `Shaders_5_0_PP.shdc` | 108 | 213 | post-process |
| `Shaders_5_0_CS.shdc` | 5 | 5 | compute |
| `Shaders_5_0_GS.shdc` | 1 | 1 | geometry |

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

Entry point names are useless here: 19 names cover 3632 compiled blobs, most of
them sharing `VSShaderPool`, because they are permutations of one uber-shader
compiled with different defines. Input signatures do separate them. All 3632
blobs parse, and they use only **17 distinct vertex formats**.

Four of those seventeen account for 3602 blobs, or 99.2% of the container, and
they are a clean two-by-two: static or skinned, instanced or not.

| blobs | vertex format | meaning |
|--:|---|---|
| 922 | `POSITION, NORMAL, TEXCOORD, TEXCOORD1, TANGENT, TANGENT1` | static mesh |
| 920 | the above + `instTransform0-2, instColor, instShadow` | static, instanced |
| 920 | static + `BLENDWEIGHT, BLENDINDICES` + instancing | skinned, instanced |
| 840 | static + `BLENDWEIGHT, BLENDINDICES` | skinned mesh |

That is the whole permutation space for ordinary geometry. The uber-shader
varies over skinning and instancing, and nothing else about vertex input.

The remaining 30 blobs are specialists:

| blobs | vertex format | likely use |
|--:|---|---|
| 15 | `POSITION, NORMAL, TEXCOORD, TANGENT, TANGENT1, POSITION1, POSITION2, TEXCOORD2` | multi-position, morph or shadow extrusion |
| 2 | `POSITION, TEXCOORD, COLOR` | simple coloured geometry |
| 2 | `POSITION, TEXCOORD, TEXCOORD1, TEXCOORD2, TEXCOORD3` | four UV sets, terrain or layered blend |
| 2 | `TEXCOORD, instPosition, instColor, instMapping_Angle, instEmissiveColor` | instanced sprites and lights |
| 1 | `POSITION, COLOR, COLOR1, TEXCOORD, TEXCOORD1` | particles |
| 1 | `POSITION, NORMAL` | depth or shadow only |
| 1 | `POSITION, NORMAL, instTransform0-2, animTextureOffset` | animated-texture instancing |
| 1 each | three `POSITION/BLENDWEIGHT/BLENDINDICES` sets, with and without instancing | multi-bone or cloth |

### What this means for modelling

- Models supply **two UV sets** and **two tangent streams**. The second UV set
  is real vertex data, not something a material synthesises.
- Skinning is four-weight, via `BLENDWEIGHT` and `BLENDINDICES`.
- Instancing is a vertex-level feature carrying its own transform, colour, and
  a dedicated `instShadow` attribute. No material property turns it on.
- Four-UV and multi-position formats exist but are used by a handful of shaders,
  so they are engine-internal rather than something a material can request.

Constant buffer counts were not reliably readable for the pooled vertex blobs
and are omitted rather than guessed.

## Post-process
These are the screen-space effects, and they are the least documented part of
the renderer: 108 entry points across 213 compiled blobs, none of them
addressable from a material. They run as engine passes. Grouping is by name.

Instruction counts are given only where the texture and constant-buffer pair
identifies a single blob; a blank means several blobs share that shape and the
count would be a guess.

### Anti-aliasing (7)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `A3_SMAABlendingWeightCalculationPS` | 3 | 1 |  |
| `A3_SMAAColorEdgeDetectionPS` | 2 | 0 |  |
| `A3_SMAANeighborhoodBlendingPS` | 2 | 1 |  |
| `DbgDisplayEdgesPS` | 1 | 0 |  |
| `Edges0PS` | 1 | 1 |  |
| `Edges1PS` | 1 | 1 |  |
| `EdgesCombinePS` | 1 | 1 |  |

### God rays (6)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `PSGodRaysApplyClouds` | 2 | 0 |  |
| `PSGodRaysBlur` | 1 | 1 |  |
| `PSGodRaysCompose` | 1 | 1 |  |
| `PSGodRaysDownscaleH` | 1 | 1 |  |
| `PSGodRaysDownscaleV` | 1 | 1 |  |
| `PSGodRaysLightSource` | 0 | 1 |  |

### Bloom (7)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `PSPostProcessBloomCombine` | 2 | 1 |  |
| `PSPostProcessBloomDownsample2` | 2 | 2 |  |
| `PSPostProcessBloomDownsample4` | 1 | 1 |  |
| `PSPostProcessGlowNewBloomCompose` | 1 | 0 |  |
| `PSPostProcessGlowNewBloomInitDownsample` | 1 | 1 |  |
| `PSPostProcessGlowNewBloomInitDownsample4x` | 4 | 1 |  |
| `PSPostProcessGlowNewBloomKawase` | 1 | 1 |  |

### Depth of field (2)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `PSPostProcessDOF` | 3 | 1 |  |
| `PSPostProcessDistanceDOF` | 3 | 1 |  |

### Exposure and tone (13)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `PSPostProcessAssumedLuminance` | 2 | 1 |  |
| `PSPostProcessAssumedLuminanceDirect` | 1 | 1 |  |
| `PSPostProcessColorsSimple` | 1 | 1 |  |
| `PSPostProcessDownSampleAvgLuminance` | 4 | 0 |  |
| `PSPostProcessDownSampleLuminance` | 4 | 0 |  |
| `PSPostProcessDownSampleMaxAvgMinLuminance` | 4 | 0 |  |
| `PSPostProcessGlowNewLuminanceAvg2x` | 1 | 0 |  |
| `PSPostProcessGlowNewLuminanceAvg4x` | 4 | 0 |  |
| `PSPostProcessGlowNewLuminanceInit` | 4 | 1 |  |
| `PSPostProcessHDRColorLimit` | 1 | 1 |  |
| `PSPostProcessHDRColorLimitNVG` | 1 | 1 |  |
| `PsPpColors` | 1 | 1 |  |
| `PsPpColorsRadial` | 2 | 1 |  |

### Blur and filter (15)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `PSFilterX` | 2 | 1 |  |
| `PSFilterY` | 2 | 1 |  |
| `PSPostProcessGaussBlur` | 1 | 1 |  |
| `PSPostProcessGaussianBlurH` | 1 | 1 |  |
| `PSPostProcessGaussianBlurV` | 1 | 1 |  |
| `PSPostProcessSSAOBlurHorz` | 1 | 1 |  |
| `PSPostProcessSSAOBlurHorzDepth` | 2 | 1 |  |
| `PSPostProcessSSAOBlurVert` | 1 | 1 |  |
| `PSPostProcessSSAOBlurVertDepth` | 2 | 1 |  |
| `PSSSAOBlurHoriz` | 2 | 1 |  |
| `PSSSAOBlurVert` | 2 | 1 |  |
| `PsPpDynamicBlur` | 1 | 1 |  |
| `PsPpDynamicBlurFinal` | 2 | 0 |  |
| `PsPpRadialBlur` | 1 | 1 |  |
| `PsPpRotBlur` | 2 | 0 |  |

### Water and caustics (2)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `PSPostProcessCaustics` | 2 | 2 |  |
| `PsPpWetDistort` | 2 | 1 |  |

### Vision modes (8)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `PSPostProcessGlowNewFinalNVG` | 2 | 1 |  |
| `PSPostProcessGlowNewFinalNightFilmic` | 2 | 1 |  |
| `PSPostProcessGlowNewFinalNightNone` | 2 | 1 |  |
| `PSPostProcessGlowNewFinalNightReinhard` | 2 | 1 |  |
| `PSPostProcessGlowNight` | 4 | 2 |  |
| `PSPostProcessNVG` | 3 | 2 | 17 |
| `PSPostProcessThermal` | 2 | 2 |  |
| `PSPostProcessThermalPresent` | 2 | 1 |  |

### Ambient occlusion (6)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `PSHDAO` | 1 | 1 |  |
| `PSPostProcessSSAO` | 2 | 1 |  |
| `PSPostProcessSSAODebug` | 1 | 0 |  |
| `PSPostProcessSSAODown` | 1 | 0 |  |
| `PSPostProcessSSAOFinal` | 1 | 0 |  |
| `PSSSAO` | 1 | 1 |  |

### Copy and resolve (7)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `PSCopy` | 1 | 0 |  |
| `PSPostProcessCopy` | 1 | 0 |  |
| `PSPostProcessCopyAlphaOne` | 1 | 0 |  |
| `PSPostProcessCopyResolve` | 1 | 1 |  |
| `PSPostProcessCopyResolveAlphaOne` | 1 | 1 |  |
| `PSPostProcessDepthResolveFirstSample` | 1 | 0 |  |
| `PSPostProcessDepthResolveMax` | 1 | 0 |  |

### Debug and diagnostic (2)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `PSOcclusionQueryDebug` | 0 | 0 |  |
| `PSPostProcessDiagDepthBuffer` | 1 | 1 |  |

### Other (33)
| entry point | tex | cb | instr |
|---|--:|--:|--:|
| `PSPostProcessFilmGrainColor` | 2 | 1 |  |
| `PSPostProcessFilmGrainMono` | 2 | 1 |  |
| `PSPostProcessFisheye` | 1 | 1 |  |
| `PSPostProcessGlow` | 4 | 2 |  |
| `PSPostProcessGlowNewFinalFilmic` | 2 | 1 |  |
| `PSPostProcessGlowNewFinalNone` | 2 | 1 |  |
| `PSPostProcessGlowNewFinalReinhard` | 2 | 1 |  |
| `PSPostProcessGlowNewQuery` | 0 | 0 |  |
| `PSPostProcessQuery` | 2 | 0 |  |
| `PSPostProcessRain3D` | 2 | 1 |  |
| `PSPostProcessRainParticles` | 3 | 1 |  |
| `PSPostProcessRescaleBicubic` | 2 | 1 |  |
| `PSPostProcessSSSMHigh` | 2 | 2 |  |
| `PSPostProcessSSSMLow` | 2 | 1 |  |
| `PSPostProcessSSSMNormal` | 2 | 2 |  |
| `PSPostProcessSSSMStencil` | 0 | 0 |  |
| `PSPostProcessSSSMVeryHigh` | 2 | 2 |  |
| `PSPostProcessSharpen` | 1 | 1 |  |
| `PSPostProcessSimulWeather` | 1 | 0 |  |
| `PSPostProcessSimulWeatherDepthDecimate` | 1 | 1 |  |
| `PSPostProcessSimulWeatherDepthDecimateHorizontal` | 1 | 1 |  |
| `PSPostProcessSimulWeatherDepthDecimateVertical` | 1 | 1 |  |
| `PSPrepareSSDepthMap` | 1 | 1 |  |
| `PSPrepareSSDepthMapFromDepthInfo` | 1 | 0 |  |
| `PSPrepareSSSceneTexture` | 1 | 0 |  |
| `PSRainOccluderGrowH` | 1 | 0 |  |
| `PSRainOccluderGrowV` | 1 | 0 |  |
| `PSRainRefractAdjust` | 1 | 1 |  |
| `ProcessAndApplyPS` | 2 | 1 |  |
| `PsFxaa3_11` | 1 | 1 |  |
| `PsFxaa3_11Luma` | 1 | 0 |  |
| `PsPpChromAber` | 1 | 1 |  |
| `PsPpClrInversion` | 1 | 1 |  |



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

93 of the 408 pixel shaders have an official description. The remaining 315
were described by translating each shader out of its compiled form and reading
the result, rather than by guessing from the name.

Each blob was converted from DXBC to SPIR-V and then to GLSL. All 408 converted
without error. The translated code was then analysed for facts that survive
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