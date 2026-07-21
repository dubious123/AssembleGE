# C++ TODO (AGE)

## On MSVC Update (Toolset / STL / Compiler behavior)
### 1. custom static assert message 
- move run_sys static_assert to pipe, adaptor, on_ctx ... 
### 2. variadic indexing
- re-impl meta::variadic_at, index, ...

## mikktspace 
### remove external and move to test

## external wrapper
### 1. directx Math 

### 3. dxcapi shader compiler
### 4. dx12
### 5. windows

## support arm arch 

## meta cleanup

## meshlet lod (+point cloud mode)

## rewrite mikk tspace

## data_structure benchmark

## better macro, (a,b,c) -> (a)(b)(c) to reduce macro parameter size

## do pgo

## imple raw input

## lights!!
### 5. area light 

### AS Culling 중복 제거
- depth prepass AS와 opaque AS가 동일한 frustum/cone culling을 2회 수행
- depth prepass AS에서 visible mask를 버퍼에 저장, opaque AS에서 재사용
- 현재 AS 병목은 아님 (PIX 기준 수십μs), 우선순위 낮음

### add assertion 
- light sorting할때 thread 수가 wave * wave 보다 크면 reduce 1번으로 안되기 때문에 문제가 됨. assert 걸어야함
- wave 수가 thread 수보다 많다고 가정함. 안그러면 index 넘어섬
-     if (WaveIsFirstLane())
    {
        histogram_sum_arr[wave_id] = wave_histogram_sum;
    }
- wave 수가 bin_count보다 많다고 가정함

### Global error handler 

### visibility buffer

### mesh rt index buffer 크기 최적화 
full primitive대신 meshlet id map 저장

### todo - better editor 
- copy, paste
- rename assets 
- undo, redo 
- logger
- enable, disable entity



### 반사, 굴절 
soft shadow
RTAO
RT specular (reflection)


### 알려진 증상 
두 selected object가 screen space에 겹쳐있을때 selected outline이 비 결정적임. 
depth test를 추가하면 해결되지만... 거의 editor에서만 사용될것 같은 기능에 (rts나 뭐 그런거 아니면) 
너무 과분한것 같아서 일단은 그냥 둠.

### optimization
Improve Shader Performance and In-Game Frame Rates with Shader Execution Reordering 
NVIDIA RTX Kit
NVIDIA RTX Mega Geometry

runtime light_bin config : scene에 따라, 현재 상황따라 최적의 light bin center랑 extent등을 계산


### bug
multi select + delete 시 crash

### ddgi
1차 구현은 얼추 됨. 
border 구분을 못 감추겠음
수렴이 잘 안됨 ( 계속 움직임 )
scene에 따라 grid spacing을 계속 조절해줘야함 ... => full dynamic의 느낌이 안삼
visibility가 확률 모델이라서 wall이 얇으면 (spacing보다 얇으면) 빛이 샘 
근데 level에 따라 spacing은 가변임 => 막을수가 없음
emissive mat이랑 wall이 있고, 그 사이에 probe가 없으면, 망함

결국 격자구조 probe의 한계

=> surfel 시도

### surfel 
1차 완성 
todo 
- corner에서 inside wall surfel들이 영향을 너무 크게 줌 
- 특정 상황에서 fps가 나락을 감. cell 에 surfel 개수가 너무 몰려있는것 같은데 아직 조건을 모르겠음 
- probe fallback이 있으면 화면 전환등의 상황에서 도움이 될것 같음

- tile당 surfel 수가 너무 많음 256을 넘어감. 
- 화면에 안보이는 surfel끼리 그냥 계속 add ref로 살아남음. 특히 밀실에 있는 surfel끼리 그냥 난리가 남
- corner에 surfel들이 계속 생성되는듯? 


- 성능 하락의 1순위 : cell 당 surfel 개수 폭발 
- 특히 cam을 뒤로 움직이면서 벽을 통과하면, empty상태에서 surfel들이 폭발적으로 생성이 됨. 
- 이는 coverage가 0이면 거의 확정적으로 surfel이 생성되기 때문인데... 
- spawn시에 coverage == 0일때 가중치를 주고 (prob를 1.f 대신 *100 정도만 줘도 어느정도 해결된다)
- cell loop에 min(128) 정도를 주면 대부분의 상황에서는 해결이 된다. 
- 다만 light bleeding은 더 심해지고, 
- corner를 돈다거나 할때마다 빈공간 수렴은 더 어려워진다. 
- 가장 근본적인 해답은 cell당 surfel을 줄이는건데... 
- surfel을 screen space와 world space로 나누어서 cell등록은 world space surfel만 하고 
- screen space surfel의 ray에서 world space 를 spawn하는식으로 가고 (낮은 확율로) 
- screen space surfel은 screen tile을 기준으로 관리해서 screen마다 32개, 128개 이런식으로 budget을 주면 좀 나아질수도 
- 근데 이러면 그냥 screen space gi랑 뭐가 다르지

- 생각해보면 screen space surfel들은 gi_resolve_buffer를 생성하고 나면 더이상 쓸모가 없음 
- 전체 surfel budget중 한 50%를 screen space surfel로 배정해서 따로 stack에 다 복사해오고, 그다음 screen tile마다 alloc해버린다면?
- 일단 ray trace + radiance sharing pass때 screen space srufel들을 순회하지 않아도 됨 


- cell 당 surfel 순회를 128max로 잡아두면 대충 fps는 min 400정도로 잡히는데, 
- 문제는 cell당 128을 넘어가면 tile 경계가 눈에 보임. 



# gibs 2.0
todo
- cell surfel의 visibility 수정, chebyshev 방식으로는 light bleeding이 너무 큼 
- 신호 freq 기반 surfel radius? 
- gtao + taa 
- visibility buffer
- unified_light를 pos + others로 분리

# morton -> hilbert 
morton cell size config 추가, 지금은 4.f 로 고정. 나중에 scene 에 따라서 동적으로 조절 가능
light pos 와 light 를 분리, 


# surfel 2.0
- cell surfel과 tile surfel의 구분 
- 시간누적추가, per pixel 누적 추가 
- motion buffer, ao추가 
- transparent support 

- emissive경우 noise는 줄어들었지만, shadow보존이 어려움 
- geo freq가 surfel freq보다 커질수록 noise가 감당이 안되고, 특히 light bleeding이 커진다. 
- emissive는 gi_only, as_point_light, as_area_light (need denoisor) 등으로 구분해야하고 
- raster 단계에서도 lighting은 결국 random sampling + 시간누적이 들어가게될듯 
- shadow ray를 1spp 로 제한하고 denoise 해야할듯


# denoiser 를 main pass에 추가, 1 directinoal light shadow (jitter) + 1 NEE 
1. gibs surfel placement (spawn/kill 진동 억제, weight 기반)
2. reflection (+ denoiser 구조를 신호 타입 파라미터화)
3. gltf scene import
4. (측정) 실제 씬에서 전체 품질 평가
5. shadow denoiser + soft shadow
6. area NEE + emissive_mode
7. GIBS 재평가 (guide, a-trous iter, conn, density) 
8. animation / skinning
9. volumetric / decal / godray





# surfel_probe
ray를 안쏜다. 
gather point, 그이상 그 이하도 아님 
다만 irradiance가 방향정보를 가짐

probe는 ddgi probe가 아님. 각각이 radiance의 역할을 한다. (irradiance가 아님) 
방향별 radiance와 
방향별 surfel coverage를 저장한다 


cell surfel과 surfel_probe가 irradiance와 radiance의 역할을 둘다한다. 
같은 표면에서 query할때는 radius를 고려한 irradiance를 얻지만, 
다른 표면에서 query할때는 facing surfel, probe의 radiance를 얻는다. 
이때 해당 지점에서의 radiance값이 필요하기 때문에 모든 cell surfel과 surfel_probe는 매 프레임 di를 계산해야한다. 
(tile surfel은 기존 구조 유지) 

따라서 tile surfel과 cell surfel은 서로 다른 구조를 유지하게됨. 
di계산시 specular 는 적당히 계산하거나 생략한다. 


따라서 기존의 sh방식의 radiance는 폐기, 
다만 sh정보를 활용한 depth와 coverage는 유지.



## spawn

소비자(RT hit / transparent PS)가 셀의 probe 순회 시 coverage 부족 

## kill

ref 타임아웃 (소비자가 안 읽음, 16~32 프레임 — 짧게)
과밀 확률 kill 
hit 지점에 max contribution + probe_id push, 
atomic으로 contribution이 가장 큰 개체 1개만 

cell당 surfel_probe_min_coverage랑 surfel_probe_max_coverage 필요 

## update

자기 cell에 등록된 surfel들을 순회하면서 누적한다. 
시간누적필요? denoise 필요? 

# cell_surfel
## spawn

probe 의 gather coverage 가 0일 때 probe 자기 위치 아래 (starvation 방지, 레이 불필요, seed surfel)
ray hit 지점에서 probe 순회시, 방향별 coverage가 부족하면 해당 위치에 spawn

## kill

cell surfel당 probe를 순회하면서 자신의 contribution을 계산함, cell 에 cas로 kill submit
contribution이 0면 (아무런 probe에 영향을 못줌) frame_since_ref 증가, 추후 update surfel에서 죽음

즉 cell당 cell_surfel_min_coverage 랑 cell_surfel_max_coverage 둘다 필요 

## update 

ray trace한후 각 hit pos에 probe들을 순회해서 누적한다. di는 직접계산

### cell spawn kill buffer 
1. ref array (per cell)
cell surfel ref array랑 
surfel probe ref array 
2. 

cell 에 surfel, probe intersect를 좀더 촘촘하게 하면 최적화가 될듯



object_id -> object_render_data 조회시, mesh 가 있다가 없으면 오류날거임. 
invalid id로 초기화 필요 