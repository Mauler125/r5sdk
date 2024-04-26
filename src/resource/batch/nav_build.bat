REM Build NavMesh for all levels.
recast -console levels\mp_lobby.obj 1
recast -console levels\mp_rr_aqueduct.obj 1
recast -console levels\mp_rr_arena_composite.obj 1
recast -console levels\mp_rr_arena_skygarden.obj 1
recast -console levels\mp_rr_ashs_redemption.obj 1
recast -console levels\mp_rr_canyonlands_64k_x_64k.obj 1
recast -console levels\mp_rr_canyonlands_mu1.obj 1
recast -console levels\mp_rr_canyonlands_mu1_night.obj 1
recast -console levels\mp_rr_canyonlands_staging.obj 1
recast -console levels\mp_rr_desertlands_64k_x_64k.obj 1
recast -console levels\mp_rr_desertlands_64k_x_64k_tt.obj 1
recast -console levels\mp_rr_party_crasher.obj 1

REM Copy NavMesh for identical levels.
copy /y /v %~dp0..\maps\navmesh\mp_rr_aqueduct_small.nm       %~dp0..\maps\navmesh\mp_rr_aqueduct_night_small.nm
copy /y /v %~dp0..\maps\navmesh\mp_rr_aqueduct_med_short.nm   %~dp0..\maps\navmesh\mp_rr_aqueduct_night_med_short.nm
copy /y /v %~dp0..\maps\navmesh\mp_rr_aqueduct_medium.nm      %~dp0..\maps\navmesh\mp_rr_aqueduct_night_medium.nm
copy /y /v %~dp0..\maps\navmesh\mp_rr_aqueduct_large.nm       %~dp0..\maps\navmesh\mp_rr_aqueduct_night_large.nm
copy /y /v %~dp0..\maps\navmesh\mp_rr_aqueduct_extra_large.nm %~dp0..\maps\navmesh\mp_rr_aqueduct_night_extra_large.nm

copy /y /v %~dp0..\maps\navmesh\mp_rr_desertlands_64k_x_64k_small.nm       %~dp0..\maps\navmesh\mp_rr_desertlands_64k_x_64k_nx_small.nm
copy /y /v %~dp0..\maps\navmesh\mp_rr_desertlands_64k_x_64k_med_short.nm   %~dp0..\maps\navmesh\mp_rr_desertlands_64k_x_64k_nx_med_short.nm
copy /y /v %~dp0..\maps\navmesh\mp_rr_desertlands_64k_x_64k_medium.nm      %~dp0..\maps\navmesh\mp_rr_desertlands_64k_x_64k_nx_medium.nm
copy /y /v %~dp0..\maps\navmesh\mp_rr_desertlands_64k_x_64k_large.nm       %~dp0..\maps\navmesh\mp_rr_desertlands_64k_x_64k_nx_large.nm
copy /y /v %~dp0..\maps\navmesh\mp_rr_desertlands_64k_x_64k_extra_large.nm %~dp0..\maps\navmesh\mp_rr_desertlands_64k_x_64k_nx_extra_large.nm
