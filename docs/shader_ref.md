# Arma 3 pixel shader reference

Measured from `Dta/bin.pbo` of a retail install. Counts are from each shader's
DXBC reflection data; stages are read from the shader's own name, which encodes them.

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
| `DepthTerrain` | 0 | 0 | 6 |  |  |
| `DetailSpecularAlpha` | 3 | 3 | 373 | S2 _dt, S5 _smdi |  |
| `DetailSpecularAlphaMacroAS` | 5 | 3 | 388 | S3 _mc + S4 _as, S2 _dt, S5 _smdi |  |
| `Empty` ✱ | 0 | 0 | 2 |  | empty shader, does not output anything (used only for depth output) |
| `Glass` ✱ | 4 | 4 | 391 |  | glass shader with environmental map |
| `Interpolation` ✱ | 2 | 4 | 182 |  |  |
| `InterpolationAlpha` ✱ | 2 | 0 | 17 |  |  |
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
| `NormalMapSpecularDIMapThermal` | 3 | 4 | 225 | S5 _smdi+DI, S1 _nohq, thermal |  |
| `NormalMapSpecularMap` ✱ | 4 | 3 | 386 | S5 _smdi, S1 _nohq | normal map with specular map |
| `NormalPiP` ✱ | 2 | 3 | 307 |  | shader for PiP screens |
| `Reflect` | 1 | 0 | 11 |  |  |
| `ReflectNoShadow` | 1 | 0 | 10 |  |  |
| `Refract` ✱ | 8 | 5 | 142 |  | shader for refractions _ARMA3_REFRACTION |
| `Road` ✱ | 5 | 5 | 528 |  | road shader |
| `Road2Pass` ✱ | 1 | 3 | 293 |  | road shader - second pass |
| `SRDetailSpecularAlphaMacroAS_Default` | 6 | 3 | 400 | S3 _mc + S4 _as, S2 _dt, S5 _smdi |  |
| `SRDetailSpecularAlpha_Default` | 4 | 3 | 383 | S2 _dt, S5 _smdi |  |
| `SRGlass_Default` | 5 | 4 | 402 |  |  |
| `SRMulti_Default` | 17 | 5 | 636 |  |  |
| `SRNormalDXTA_Default` | 3 | 4 | 379 |  |  |
| `SRNormalMapDetailMacroASSpecularDIMap_Default` | 8 | 3 | 430 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as, S2 _dt |  |
| `SRNormalMapDetailMacroASSpecularMap_Default` | 8 | 3 | 429 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as, S2 _dt |  |
| `SRNormalMapDetailSpecularDIMap_Default` | 6 | 3 | 412 | S5 _smdi+DI, S1 _nohq, S2 _dt |  |
| `SRNormalMapDetailSpecularMap_Default` | 6 | 3 | 411 | S5 _smdi, S1 _nohq, S2 _dt |  |
| `SRNormalMapMacroASSpecularDIMap_Default` | 7 | 3 | 425 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as |  |
| `SRNormalMapMacroASSpecularMap_Default` | 7 | 3 | 424 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as |  |
| `SRNormalMapMacroAS_Default` | 6 | 3 | 395 | S1 _nohq, S3 _mc + S4 _as |  |
| `SRNormalMapSpecularDIMap_Default` | 5 | 3 | 407 | S5 _smdi+DI, S1 _nohq |  |
| `SRNormalMapSpecularMap_Default` | 5 | 3 | 406 | S5 _smdi, S1 _nohq |  |
| `SRNormalMap_Default` | 5 | 3 | 389 | S1 _nohq |  |
| `SRNormalPiP_Default` | 2 | 3 | 307 |  |  |
| `SRRefract_Default` | 8 | 5 | 166 |  |  |
| `SRRoad2Pass_Default` | 1 | 3 | 293 |  |  |
| `SRRoad_Default` | 6 | 5 | 538 |  |  |
| `SRSkin_Default` | 9 | 6 | 471 |  |  |
| `SRSpecularAlpha_Default` | 3 | 3 | 378 | S5 _smdi |  |
| `SRSpecularNormalMapDiffuseMacroAS_Default` | 7 | 3 | 412 | S1 _nohq, S3 _mc + S4 _as, S5 _smdi |  |
| `SRSpecularNormalMapDiffuse_Default` | 5 | 3 | 394 | S1 _nohq, S5 _smdi |  |
| `SRSpecularNormalMapSpecularThroughSimple_Default` | 4 | 4 | 396 | S1 _nohq, S5 _smdi, see-through, reduced |  |
| `SRSpecularNormalMapSpecularThrough_Default` | 6 | 4 | 405 | S1 _nohq, S5 _smdi, see-through |  |
| `SRSpecularNormalMapThroughSimple_Default` | 4 | 4 | 384 | S1 _nohq, S5 _smdi, see-through, reduced |  |
| `SRSpecularNormalMapThrough_Default` | 6 | 4 | 392 | S1 _nohq, S5 _smdi, see-through |  |
| `SRSuperAToC_Default` | 10 | 6 | 574 | alpha-to-coverage |  |
| `SRSuperExt_Default` | 11 | 6 | 588 |  |  |
| `SRSuperHairAtoC_Default` | 10 | 7 | 591 |  |  |
| `SRSuperHair_Default` | 10 | 7 | 588 |  |  |
| `SRSuper_Default` | 10 | 6 | 584 |  |  |
| `SRTerrain15_Default` | 13 | 6 | 1044 |  |  |
| `SRTerrainNoDetailSNX_Default` | 5 | 6 | 454 | S2 _dt |  |
| `SRTerrainNoDetailX_Default` | 4 | 6 | 437 | S2 _dt |  |
| `SRTerrainSNX_Default` | 16 | 6 | 1215 |  |  |
| `SRTerrainSimple15_Default` | 13 | 6 | 570 | reduced |  |
| `SRTerrainSimpleSNX_Default` | 16 | 6 | 619 | reduced |  |
| `SRTerrainSimpleX_Default` | 17 | 6 | 613 | reduced |  |
| `SRTerrainX_Default` | 17 | 6 | 1316 |  |  |
| `SSSMDetailSpecularAlpha` | 4 | 3 | 384 | S2 _dt, S5 _smdi |  |
| `SSSMDetailSpecularAlphaMacroAS` | 6 | 3 | 401 | S3 _mc + S4 _as, S2 _dt, S5 _smdi |  |
| `SSSMGlass` | 5 | 4 | 403 |  |  |
| `SSSMMulti` | 17 | 5 | 637 |  |  |
| `SSSMNormalDXTA` | 3 | 4 | 380 |  |  |
| `SSSMNormalMap` | 5 | 3 | 390 | S1 _nohq |  |
| `SSSMNormalMapDetailMacroASSpecularDIMap` | 8 | 3 | 431 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as, S2 _dt |  |
| `SSSMNormalMapDetailMacroASSpecularMap` | 8 | 3 | 430 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as, S2 _dt |  |
| `SSSMNormalMapDetailSpecularDIMap` | 6 | 3 | 413 | S5 _smdi+DI, S1 _nohq, S2 _dt |  |
| `SSSMNormalMapDetailSpecularMap` | 6 | 3 | 412 | S5 _smdi, S1 _nohq, S2 _dt |  |
| `SSSMNormalMapMacroAS` | 6 | 3 | 396 | S1 _nohq, S3 _mc + S4 _as |  |
| `SSSMNormalMapMacroASSpecularDIMap` | 7 | 3 | 426 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as |  |
| `SSSMNormalMapMacroASSpecularMap` | 7 | 3 | 425 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as |  |
| `SSSMNormalMapSpecularDIMap` | 5 | 3 | 408 | S5 _smdi+DI, S1 _nohq |  |
| `SSSMNormalMapSpecularMap` | 5 | 3 | 407 | S5 _smdi, S1 _nohq |  |
| `SSSMNormalPiP` | 2 | 3 | 307 |  |  |
| `SSSMRefract` | 8 | 5 | 166 |  |  |
| `SSSMRoad` | 6 | 5 | 539 |  |  |
| `SSSMRoad2Pass` | 1 | 3 | 293 |  |  |
| `SSSMSkin` | 9 | 6 | 458 |  |  |
| `SSSMSpecularAlpha` | 3 | 3 | 379 | S5 _smdi |  |
| `SSSMSpecularNormalMapDiffuse` | 5 | 3 | 395 | S1 _nohq, S5 _smdi |  |
| `SSSMSpecularNormalMapDiffuseMacroAS` | 7 | 3 | 413 | S1 _nohq, S3 _mc + S4 _as, S5 _smdi |  |
| `SSSMSpecularNormalMapSpecularThrough` | 6 | 4 | 406 | S1 _nohq, S5 _smdi, see-through |  |
| `SSSMSpecularNormalMapSpecularThroughSimple` | 4 | 4 | 397 | S1 _nohq, S5 _smdi, see-through, reduced |  |
| `SSSMSpecularNormalMapThrough` | 6 | 4 | 393 | S1 _nohq, S5 _smdi, see-through |  |
| `SSSMSpecularNormalMapThroughSimple` | 4 | 4 | 385 | S1 _nohq, S5 _smdi, see-through, reduced |  |
| `SSSMSuper` | 10 | 6 | 585 |  |  |
| `SSSMSuperAToC` | 10 | 6 | 575 | alpha-to-coverage |  |
| `SSSMSuperExt` | 11 | 6 | 589 |  |  |
| `SSSMSuperHair` | 10 | 7 | 589 |  |  |
| `SSSMSuperHairAtoC` | 10 | 7 | 592 |  |  |
| `SSSMTerrain15` | 13 | 6 | 1045 |  |  |
| `SSSMTerrainNoDetailSNX` | 5 | 6 | 455 | S2 _dt |  |
| `SSSMTerrainNoDetailX` | 4 | 6 | 438 | S2 _dt |  |
| `SSSMTerrainSNX` | 16 | 6 | 1216 |  |  |
| `SSSMTerrainSimple15` | 13 | 6 | 571 | reduced |  |
| `SSSMTerrainSimpleSNX` | 16 | 6 | 630 | reduced |  |
| `SSSMTerrainSimpleX` | 17 | 6 | 614 | reduced |  |
| `SSSMTerrainX` | 17 | 6 | 1317 |  |  |
| `ShadowBufferAlpha` | 1 | 0 | 10 |  |  |
| `Skin` ✱ | 8 | 6 | 445 |  | Human skin - derived from Super shader |
| `SpecularAlpha` | 2 | 3 | 368 | S5 _smdi |  |
| `SpecularAlphaThermal` | 3 | 4 | 383 | S5 _smdi, thermal |  |
| `SpecularNormalMapDiffuse` | 4 | 3 | 384 | S1 _nohq, S5 _smdi |  |
| `SpecularNormalMapDiffuseMacroAS` | 6 | 3 | 400 | S1 _nohq, S3 _mc + S4 _as, S5 _smdi |  |
| `SpecularNormalMapSpecularThrough` | 5 | 4 | 397 | S1 _nohq, S5 _smdi, see-through |  |
| `SpecularNormalMapSpecularThroughSimple` | 3 | 4 | 378 | S1 _nohq, S5 _smdi, see-through, reduced |  |
| `SpecularNormalMapThrough` | 5 | 4 | 384 | S1 _nohq, S5 _smdi, see-through |  |
| `SpecularNormalMapThroughSimple` | 3 | 4 | 376 | S1 _nohq, S5 _smdi, see-through, reduced |  |
| `Super` ✱ | 9 | 6 | 572 |  | Super shader |
| `SuperAToC` ✱ | 9 | 6 | 572 | alpha-to-coverage | Super shader AToC variant |
| `SuperAToCThermal` | 7 | 4 | 418 | thermal, alpha-to-coverage |  |
| `SuperExt` ✱ | 10 | 6 | 576 |  | skyscraper & building, intended as super shader light version |
| `SuperHair` ✱ | 9 | 7 | 578 |  | super shader for hair rendering |
| `SuperHairAtoC` ✱ | 9 | 7 | 579 |  | super shader for hair rendering, atoc version |
| `SuperThermal` | 7 | 4 | 421 | thermal |  |
| `ThermalCrater1` | 2 | 6 | 201 | thermal |  |
| `ThermalCrater10` | 2 | 6 | 255 | thermal |  |
| `ThermalCrater11` | 2 | 6 | 261 | thermal |  |
| `ThermalCrater12` | 2 | 6 | 267 | thermal |  |
| `ThermalCrater13` | 2 | 6 | 273 | thermal |  |
| `ThermalCrater14` | 2 | 6 | 279 | thermal |  |
| `ThermalCrater2` | 2 | 6 | 207 | thermal |  |
| `ThermalCrater3` | 2 | 6 | 213 | thermal |  |
| `ThermalCrater4` | 2 | 6 | 219 | thermal |  |
| `ThermalCrater5` | 2 | 6 | 225 | thermal |  |
| `ThermalCrater6` | 2 | 6 | 231 | thermal |  |
| `ThermalCrater7` | 2 | 6 | 237 | thermal |  |
| `ThermalCrater8` | 2 | 6 | 243 | thermal |  |
| `ThermalCrater9` | 2 | 6 | 249 | thermal |  |
| `ThermalDetailSpecularAlpha` | 3 | 4 | 226 | S2 _dt, S5 _smdi, thermal |  |
| `ThermalDetailSpecularAlphaMacroAS` | 4 | 4 | 229 | S3 _mc + S4 _as, S2 _dt, S5 _smdi, thermal |  |
| `ThermalGlass` | 2 | 4 | 221 | thermal |  |
| `ThermalInterpolation` | 0 | 0 | 4 | thermal |  |
| `ThermalInterpolationAlpha` | 3 | 1 | 42 | thermal |  |
| `ThermalMulti` | 16 | 4 | 327 | thermal |  |
| `ThermalNormalDXTA` | 2 | 5 | 223 | thermal |  |
| `ThermalNormalMap` | 3 | 4 | 223 | S1 _nohq, thermal |  |
| `ThermalNormalMapDetailMacroASSpecularDIMap` | 5 | 4 | 240 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as, S2 _dt, thermal |  |
| `ThermalNormalMapDetailMacroASSpecularMap` | 5 | 4 | 240 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as, S2 _dt, thermal |  |
| `ThermalNormalMapDetailSpecularDIMap` | 4 | 4 | 235 | S5 _smdi+DI, S1 _nohq, S2 _dt, thermal |  |
| `ThermalNormalMapDetailSpecularMap` | 4 | 4 | 235 | S5 _smdi, S1 _nohq, S2 _dt, thermal |  |
| `ThermalNormalMapMacroAS` | 4 | 4 | 230 | S1 _nohq, S3 _mc + S4 _as, thermal |  |
| `ThermalNormalMapMacroASSpecularDIMap` | 4 | 4 | 235 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as, thermal |  |
| `ThermalNormalMapMacroASSpecularMap` | 4 | 4 | 235 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as, thermal |  |
| `ThermalNormalMapSpecularMap` | 3 | 4 | 228 | S5 _smdi, S1 _nohq, thermal |  |
| `ThermalRefract` | 5 | 4 | 115 | thermal |  |
| `ThermalRoad` | 4 | 4 | 229 | thermal |  |
| `ThermalRoad2Pass` | 2 | 4 | 197 | thermal |  |
| `ThermalSkin` | 3 | 4 | 373 | thermal |  |
| `ThermalSpecularNormalMapDiffuse` | 4 | 4 | 235 | S1 _nohq, S5 _smdi, thermal |  |
| `ThermalSpecularNormalMapDiffuseMacroAS` | 5 | 4 | 240 | S1 _nohq, S3 _mc + S4 _as, S5 _smdi, thermal |  |
| `ThermalSpecularNormalMapSpecularThrough` | 4 | 5 | 235 | S1 _nohq, S5 _smdi, thermal, see-through |  |
| `ThermalSpecularNormalMapSpecularThroughSimple` | 3 | 5 | 230 | S1 _nohq, S5 _smdi, thermal, see-through, reduced |  |
| `ThermalSpecularNormalMapThrough` | 4 | 5 | 233 | S1 _nohq, S5 _smdi, thermal, see-through |  |
| `ThermalSpecularNormalMapThroughSimple` | 3 | 5 | 227 | S1 _nohq, S5 _smdi, thermal, see-through, reduced |  |
| `ThermalSuperExt` | 5 | 4 | 245 | thermal |  |
| `ThermalSuperHair` | 5 | 5 | 251 | thermal |  |
| `ThermalSuperHairAtoC` | 5 | 5 | 252 | thermal |  |
| `White` ✱ | 0 | 0 | 4 |  |  |
| `WhiteAlpha` ✱ | 1 | 0 | 9 |  |  |

## foliage (92)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `AlphaOnlyTree` | 2 | 2 | 52 |  |  |
| `AlphaOnlyTreeAToC` | 1 | 2 | 52 | alpha-to-coverage |  |
| `AlphaOnlyTreeAdv` | 1 | 2 | 48 |  |  |
| `AlphaOnlyTreeAdvAToC` | 1 | 2 | 52 | alpha-to-coverage |  |
| `DEBUGSHDGrass` | 2 | 3 | 96 |  |  |
| `DEBUGSHDGrassAToC` | 2 | 3 | 94 | alpha-to-coverage |  |
| `DEBUGSHDSpecularNormalMapGrass` | 3 | 3 | 81 | S1 _nohq, S5 _smdi |  |
| `DEBUGSHDTerrainGrass15` | 3 | 4 | 74 |  |  |
| `DEBUGSHDTerrainGrassX` | 3 | 4 | 74 |  |  |
| `DEBUGSHDTree` | 4 | 4 | 126 |  |  |
| `DEBUGSHDTreeAToC` | 4 | 4 | 126 | alpha-to-coverage |  |
| `DEBUGSHDTreeAdv` | 4 | 4 | 172 |  |  |
| `DEBUGSHDTreeAdvAToC` | 4 | 4 | 173 | alpha-to-coverage |  |
| `DEBUGSHDTreeAdvSimple` | 4 | 4 | 129 | reduced |  |
| `DEBUGSHDTreeAdvSimpleAToC` | 4 | 4 | 130 | alpha-to-coverage, reduced |  |
| `DEBUGSHDTreeAdvTrans` | 4 | 4 | 171 |  |  |
| `DEBUGSHDTreeAdvTransAToC` | 4 | 4 | 172 | alpha-to-coverage |  |
| `DEBUGSHDTreeAdvTrunk` | 4 | 4 | 160 |  |  |
| `DEBUGSHDTreeAdvTrunkSimple` | 4 | 4 | 122 | reduced |  |
| `DEBUGSHDTreePRT` | 4 | 5 | 104 |  |  |
| `DEBUGSHDTreeSN` | 4 | 4 | 125 |  |  |
| `DEBUGSHDTreeSimple` | 4 | 4 | 111 | reduced |  |
| `Grass` ✱ | 2 | 3 | 366 |  | grass shader - alpha discretized |
| `GrassAToC` ✱ | 2 | 3 | 364 | alpha-to-coverage | grass with alpha to coverage |
| `SRGrassAToC_Default` | 3 | 3 | 374 | alpha-to-coverage |  |
| `SRGrass_Default` | 3 | 3 | 376 |  |  |
| `SRSpecularNormalMapGrass_Default` | 4 | 3 | 361 | S1 _nohq, S5 _smdi |  |
| `SRTerrainGrass15_Default` | 4 | 6 | 440 |  |  |
| `SRTerrainGrassX_Default` | 4 | 6 | 440 |  |  |
| `SRTreeAToC_Default` | 5 | 4 | 398 | alpha-to-coverage |  |
| `SRTreeAdvAToC_Default` | 4 | 4 | 319 | alpha-to-coverage |  |
| `SRTreeAdvSimpleAToC_Default` | 4 | 4 | 277 | alpha-to-coverage, reduced |  |
| `SRTreeAdvSimple_Default` | 4 | 4 | 276 | reduced |  |
| `SRTreeAdvTransAToC_Default` | 4 | 4 | 319 | alpha-to-coverage |  |
| `SRTreeAdvTrans_Default` | 4 | 4 | 318 |  |  |
| `SRTreeAdvTrunkSimple_Default` | 4 | 4 | 247 | reduced |  |
| `SRTreeAdvTrunk_Default` | 4 | 4 | 285 |  |  |
| `SRTreeAdv_Default` | 4 | 4 | 318 |  |  |
| `SRTreePRT_Default` | 5 | 5 | 372 |  |  |
| `SRTreeSN_Default` | 5 | 4 | 405 |  |  |
| `SRTreeSimple_Default` | 5 | 4 | 389 | reduced |  |
| `SRTree_Default` | 5 | 4 | 406 |  |  |
| `SSSMGrass` | 3 | 3 | 377 |  |  |
| `SSSMGrassAToC` | 3 | 3 | 375 | alpha-to-coverage |  |
| `SSSMSpecularNormalMapGrass` | 4 | 3 | 362 | S1 _nohq, S5 _smdi |  |
| `SSSMTerrainGrass15` | 4 | 6 | 441 |  |  |
| `SSSMTerrainGrassX` | 4 | 6 | 431 |  |  |
| `SSSMTree` | 5 | 4 | 407 |  |  |
| `SSSMTreeAToC` | 5 | 4 | 409 | alpha-to-coverage |  |
| `SSSMTreeAdv` | 5 | 4 | 328 |  |  |
| `SSSMTreeAdvAToC` | 5 | 4 | 331 | alpha-to-coverage |  |
| `SSSMTreeAdvSimple` | 5 | 4 | 286 | reduced |  |
| `SSSMTreeAdvSimpleAToC` | 5 | 4 | 289 | alpha-to-coverage, reduced |  |
| `SSSMTreeAdvTrans` | 5 | 4 | 328 |  |  |
| `SSSMTreeAdvTransAToC` | 5 | 4 | 331 | alpha-to-coverage |  |
| `SSSMTreeAdvTrunk` | 5 | 4 | 297 |  |  |
| `SSSMTreeAdvTrunkSimple` | 5 | 4 | 259 | reduced |  |
| `SSSMTreePRT` | 5 | 5 | 383 |  |  |
| `SSSMTreeSN` | 5 | 4 | 406 |  |  |
| `SSSMTreeSimple` | 5 | 4 | 390 | reduced |  |
| `SpecularNormalMapGrass` | 3 | 3 | 351 | S1 _nohq, S5 _smdi |  |
| `ThermalGrass` | 2 | 4 | 221 | thermal |  |
| `ThermalGrassAToC` | 2 | 4 | 219 | thermal, alpha-to-coverage |  |
| `ThermalSpecularNormalMapGrass` | 2 | 4 | 212 | S1 _nohq, S5 _smdi, thermal |  |
| `ThermalTerrainGrass15` | 3 | 5 | 222 | thermal |  |
| `ThermalTerrainGrassX` | 3 | 5 | 222 | thermal |  |
| `ThermalTree` | 4 | 5 | 239 | thermal |  |
| `ThermalTreeAToC` | 4 | 5 | 239 | thermal, alpha-to-coverage |  |
| `ThermalTreeAdv` | 3 | 4 | 363 | thermal |  |
| `ThermalTreeAdvAToC` | 3 | 4 | 364 | thermal, alpha-to-coverage |  |
| `ThermalTreeAdvSimple` | 3 | 4 | 363 | thermal, reduced |  |
| `ThermalTreeAdvSimpleAToC` | 3 | 4 | 364 | thermal, alpha-to-coverage, reduced |  |
| `ThermalTreeAdvTrans` | 3 | 4 | 363 | thermal |  |
| `ThermalTreeAdvTransAToC` | 3 | 4 | 364 | thermal, alpha-to-coverage |  |
| `ThermalTreeAdvTrunk` | 3 | 3 | 356 | thermal |  |
| `ThermalTreeAdvTrunkSimple` | 3 | 3 | 356 | thermal, reduced |  |
| `ThermalTreePRT` | 3 | 5 | 226 | thermal |  |
| `ThermalTreeSN` | 4 | 5 | 238 | thermal |  |
| `ThermalTreeSimple` | 4 | 5 | 236 | thermal, reduced |  |
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
| `Terrain15Thermal` | 12 | 5 | 806 | thermal |  |
| `TerrainGrass15` ✱ | 3 | 6 | 420 |  | terrain grass - X layers |
| `TerrainGrassAlphaX` | 0 | 0 | 6 |  |  |
| `TerrainGrassX` ✱ | 3 | 6 | 420 |  | terrain grass - general number of layers |
| `TerrainNoDetailSNX` ✱ | 4 | 6 | 444 | S2 _dt | terrainSNX without detail map |
| `TerrainNoDetailSNXThermal` | 4 | 5 | 225 | S2 _dt, thermal |  |
| `TerrainNoDetailX` ✱ | 3 | 6 | 427 | S2 _dt | terrainX without detail map |
| `TerrainNoDetailXThermal` | 3 | 5 | 214 | S2 _dt, thermal |  |
| `TerrainSNX` ✱ | 15 | 6 | 1205 |  | terrain - general number of layers + satellite normal map |
| `TerrainSNXThermal` | 15 | 5 | 957 | thermal |  |
| `TerrainSimple15` ✱ | 12 | 6 | 560 | reduced | terrainSimple - X layers |
| `TerrainSimple15Thermal` | 12 | 5 | 325 | thermal, reduced |  |
| `TerrainSimpleSNX` ✱ | 15 | 6 | 619 | reduced | terrainSNX without parallax mapping |
| `TerrainSimpleSNXThermal` | 15 | 5 | 376 | thermal, reduced |  |
| `TerrainSimpleX` ✱ | 16 | 6 | 603 | reduced | terrainSimple - general number of layers |
| `TerrainSimpleXThermal` | 16 | 5 | 368 | thermal, reduced |  |
| `TerrainX` ✱ | 16 | 6 | 1306 |  | terrain - general number of layers |
| `TerrainXThermal` | 16 | 5 | 1067 | thermal |  |

## water (17)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `CalmWater` ✱ | 9 | 2 | 174 |  | calm water surface |
| `CalmWaterTi` | 1 | 0 | 19 |  |  |
| `Caustics` ✱ | 1 | 2 | 75 |  | shader for caustics effect |
| `CausticsSSSM` | 2 | 2 | 82 |  |  |
| `DEBUGSHDWater` | 10 | 5 | 1748 |  |  |
| `SRCalmWater_Default` | 11 | 3 | 211 |  |  |
| `SRWater_Default` | 10 | 7 | 1938 |  |  |
| `SSSMCalmWater` | 11 | 3 | 209 |  |  |
| `SSSMWater` | 10 | 7 | 1938 |  |  |
| `Shore` ✱ | 12 | 7 | 2099 |  | shore shader |
| `ShoreFoam` ✱ | 1 | 4 | 303 |  | shore shader for the foam on the top of the shore |
| `ShoreWet` ✱ | 0 | 3 | 293 |  | shore shader for the wet part |
| `ThermalWater` | 1 | 3 | 164 | thermal |  |
| `UnderwaterOcclusion` ✱ | 0 | 2 | 23 |  | Shader used for underwater occlusion object |
| `UnderwaterOcclusionThermal` | 0 | 0 | 6 | thermal |  |
| `Water` ✱ | 9 | 7 | 1923 |  | sea water |
| `WaterSimple` ✱ | 2 | 3 | 52 | reduced | small water |

## sky (18)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `Cloud` ✱ | 1 | 2 | 19 |  | Shader used for clouds |
| `CloudThermal` | 1 | 1 | 13 | thermal |  |
| `DEBUGSHDVolCloud` | 3 | 3 | 59 |  |  |
| `DEBUGSHDVolCloudSimple` | 2 | 3 | 55 | reduced |  |
| `Horizon` ✱ | 2 | 3 | 190 |  | Shader used for the horizon |
| `HorizonThermal` | 1 | 0 | 24 | thermal |  |
| `SRVolCloudSimple_Default` | 3 | 3 | 615 | reduced |  |
| `SRVolCloud_Default` | 4 | 3 | 619 |  |  |
| `SSSMVolCloud` | 4 | 3 | 620 |  |  |
| `SSSMVolCloudSimple` | 3 | 3 | 616 | reduced |  |
| `SimulWeatherClouds` ✱ | 7 | 2 | 97 |  | SimulWeather clouds |
| `SimulWeatherCloudsCPU` ✱ | 3 | 2 | 65 |  | SimulWeather clouds with CPU distance fading |
| `SimulWeatherCloudsWithLightning` ✱ | 8 | 2 | 107 |  | SimulWeather clouds with lightning |
| `SimulWeatherCloudsWithLightningCPU` ✱ | 4 | 2 | 75 |  | SimulWeather clouds with lightning and CPU distance fading |
| `ThermalVolCloud` | 3 | 4 | 213 | thermal |  |
| `ThermalVolCloudSimple` | 2 | 4 | 194 | thermal, reduced |  |
| `VolCloud` ✱ | 3 | 3 | 607 |  | Shader used for volumetric cloud - it uses SoftParticle approach |
| `VolCloudSimple` ✱ | 2 | 3 | 605 | reduced | Shader used for volumetric cloud - no SoftParticle approach |

## sprite (25)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `DEBUGSHDSprite` | 3 | 3 | 66 |  |  |
| `DEBUGSHDSpriteExtTi` | 3 | 3 | 66 |  |  |
| `DEBUGSHDSpriteRefract` | 4 | 3 | 65 |  |  |
| `DEBUGSHDSpriteRefractSimple` | 3 | 3 | 55 | reduced |  |
| `DEBUGSHDSpriteSimple` | 2 | 3 | 57 | reduced |  |
| `SRSpriteExtTi_Default` | 3 | 3 | 76 |  |  |
| `SRSpriteRefractSimple_Default` | 3 | 3 | 45 | reduced |  |
| `SRSpriteRefract_Default` | 4 | 3 | 55 |  |  |
| `SRSpriteSimple_Default` | 2 | 3 | 66 | reduced |  |
| `SRSprite_Default` | 3 | 3 | 76 |  |  |
| `SSSMSprite` | 3 | 3 | 76 |  |  |
| `SSSMSpriteExtTi` | 3 | 3 | 76 |  |  |
| `SSSMSpriteRefract` | 4 | 3 | 55 |  |  |
| `SSSMSpriteRefractSimple` | 3 | 3 | 45 | reduced |  |
| `SSSMSpriteSimple` | 2 | 3 | 68 | reduced |  |
| `Sprite` ✱ | 2 | 3 | 66 |  | Shader used for sprite rendering - it uses SoftParticle approach |
| `SpriteExtTi` ✱ | 2 | 3 | 66 |  | Sprite used for vehicles covering |
| `SpriteRefract` ✱ | 4 | 3 | 55 |  | _ARMA3_REFRACTION_SPRITES - Shader used for sprite rendering with refraction - it uses Sof |
| `SpriteRefractSimple` ✱ | 3 | 3 | 45 | reduced | _ARMA3_REFRACTION_SPRITES - Shader used for sprite rendering with refraction- no SoftParti |
| `SpriteSimple` ✱ | 1 | 3 | 56 | reduced | Shader used for sprite rendering - no SoftParticle approach |
| `ThermalSprite` | 3 | 4 | 69 | thermal |  |
| `ThermalSpriteExtTi` | 3 | 4 | 77 | thermal |  |
| `ThermalSpriteRefract` | 4 | 4 | 66 | thermal |  |
| `ThermalSpriteRefractSimple` | 3 | 4 | 56 | thermal, reduced |  |
| `ThermalSpriteSimple` | 2 | 4 | 59 | thermal, reduced |  |

## ui (8)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `AlphaOnly` | 1 | 1 | 43 |  |  |
| `AlphaOnlyMod` | 1 | 1 | 45 |  |  |
| `AlphaOnlyModAToC` | 1 | 1 | 45 | alpha-to-coverage |  |
| `AlphaOnlyNoAlpha` | 0 | 1 | 31 |  |  |
| `Collimator` ✱ | 1 | 3 | 291 |  | special shader for collimator |
| `CollimatorThermal` | 0 | 0 | 4 | thermal |  |
| `Point` ✱ | 0 | 3 | 297 |  | Shader used for point lights |
| `PointThermal` | 1 | 1 | 13 | thermal |  |

## debug (40)

| entry point | tex | cb | instr | stages | documented as |
|---|--:|--:|--:|---|---|
| `DEBUGSHDDetailSpecularAlpha` | 3 | 3 | 99 | S2 _dt, S5 _smdi |  |
| `DEBUGSHDDetailSpecularAlphaMacroAS` | 5 | 3 | 111 | S3 _mc + S4 _as, S2 _dt, S5 _smdi |  |
| `DEBUGSHDGlass` | 4 | 4 | 117 |  |  |
| `DEBUGSHDMulti` | 16 | 3 | 228 |  |  |
| `DEBUGSHDNormalDXTA` | 2 | 4 | 97 |  |  |
| `DEBUGSHDNormalMap` | 4 | 3 | 105 | S1 _nohq |  |
| `DEBUGSHDNormalMapDetailMacroASSpecularDIMap` | 7 | 3 | 142 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as, S2 _dt |  |
| `DEBUGSHDNormalMapDetailMacroASSpecularMap` | 7 | 3 | 141 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as, S2 _dt |  |
| `DEBUGSHDNormalMapDetailSpecularDIMap` | 5 | 3 | 128 | S5 _smdi+DI, S1 _nohq, S2 _dt |  |
| `DEBUGSHDNormalMapDetailSpecularMap` | 5 | 3 | 127 | S5 _smdi, S1 _nohq, S2 _dt |  |
| `DEBUGSHDNormalMapMacroAS` | 5 | 3 | 111 | S1 _nohq, S3 _mc + S4 _as |  |
| `DEBUGSHDNormalMapMacroASSpecularDIMap` | 6 | 3 | 137 | S5 _smdi+DI, S1 _nohq, S3 _mc + S4 _as |  |
| `DEBUGSHDNormalMapMacroASSpecularMap` | 6 | 3 | 136 | S5 _smdi, S1 _nohq, S3 _mc + S4 _as |  |
| `DEBUGSHDNormalMapSpecularDIMap` | 4 | 3 | 123 | S5 _smdi+DI, S1 _nohq |  |
| `DEBUGSHDNormalMapSpecularMap` | 4 | 3 | 122 | S5 _smdi, S1 _nohq |  |
| `DEBUGSHDNormalPiP` | 2 | 3 | 44 |  |  |
| `DEBUGSHDRefract` | 8 | 5 | 167 |  |  |
| `DEBUGSHDRoad` | 5 | 3 | 134 |  |  |
| `DEBUGSHDRoad2Pass` | 2 | 3 | 52 |  |  |
| `DEBUGSHDSkin` | 8 | 4 | 194 |  |  |
| `DEBUGSHDSpecularAlpha` | 2 | 3 | 94 | S5 _smdi |  |
| `DEBUGSHDSpecularNormalMapDiffuse` | 4 | 3 | 110 | S1 _nohq, S5 _smdi |  |
| `DEBUGSHDSpecularNormalMapDiffuseMacroAS` | 6 | 3 | 124 | S1 _nohq, S3 _mc + S4 _as, S5 _smdi |  |
| `DEBUGSHDSpecularNormalMapSpecularThrough` | 5 | 4 | 125 | S1 _nohq, S5 _smdi, see-through |  |
| `DEBUGSHDSpecularNormalMapSpecularThroughSimple` | 3 | 4 | 116 | S1 _nohq, S5 _smdi, see-through, reduced |  |
| `DEBUGSHDSpecularNormalMapThrough` | 5 | 4 | 114 | S1 _nohq, S5 _smdi, see-through |  |
| `DEBUGSHDSpecularNormalMapThroughSimple` | 3 | 4 | 106 | S1 _nohq, S5 _smdi, see-through, reduced |  |
| `DEBUGSHDSuper` | 9 | 4 | 183 |  |  |
| `DEBUGSHDSuperAToC` | 9 | 4 | 183 | alpha-to-coverage |  |
| `DEBUGSHDSuperExt` | 10 | 4 | 187 |  |  |
| `DEBUGSHDSuperHair` | 9 | 5 | 189 |  |  |
| `DEBUGSHDSuperHairAtoC` | 9 | 5 | 190 |  |  |
| `DEBUGSHDTerrain15` | 12 | 4 | 668 |  |  |
| `DEBUGSHDTerrainNoDetailSNX` | 4 | 4 | 88 | S2 _dt |  |
| `DEBUGSHDTerrainNoDetailX` | 3 | 4 | 71 | S2 _dt |  |
| `DEBUGSHDTerrainSNX` | 15 | 4 | 826 |  |  |
| `DEBUGSHDTerrainSimple15` | 12 | 4 | 187 | reduced |  |
| `DEBUGSHDTerrainSimpleSNX` | 15 | 4 | 245 | reduced |  |
| `DEBUGSHDTerrainSimpleX` | 16 | 4 | 230 | reduced |  |
| `DEBUGSHDTerrainX` | 16 | 4 | 929 |  |  |


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
