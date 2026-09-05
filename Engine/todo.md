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







far coverage 가 의미가 있으려면 near radius와 차이가 있어야함 
near radius와 차이거 거의 없다면, corner의 부분에서 의미가 생김 (국소적)
해당 차이가 성능이나 시각적인 향상이 있는지는 확인하지 못함 

near radius를 줄이고 far를 늘린다면 cell크기가 커짐. 
cell당 probe or cell_surfel의 수가 cell이 커지더라도 밀도가 일정하게 유지될거라고 예상했으나 (far coverage 때문에) 
실제로는 그러지 못함. 아직 원인은 불명. 

coverage를 극단적으로 줄이니 성능이나 cell당 할당된 probe나 cell surfel이 정상이 됨. 
near 랑 far의 fatio를 한 0.5까지는 ㄱㅊ은데 
0.1정도가면 성능이 박살남

문제는 light leaking








probe를 삭제함 
light leaking과 또 gi 에서의 anti aliasing 문제가 발생 
해결해야함.


surfel asym 검사할때 normal을 vertex normal대신 face normal로 변경해야함.

shading normal vs geo_normal 통일



aa쪽에 specular를 추가하고 
debug view 추가하고
cell surfel leak 처리하고
transparent + specular 어케할지 좀 생각하고 
아마 layer 1만 처리하고, 이후 transparent는 주사위 굴려서 시간차원 누적하면 될듯



## GIST
leak 많이 줄임

specular의 reflect ray hit에서 cell surfel이 잘 안생기는 문제가 있음. 

그래서 GIST VS GIBS 뭐가 더 남? 
잘 모르겠음. 

어두운 scene 기준 noise는 GIBS가 win 
GIST는 specular 가 있음 

GIBS가 SCREEN LEAK이 더 심함 
특히 SCENE의 거리가 클때 더 큼. 
적어도 GIST는 SCREEN만 따지면 LEAK이 거의 없음 

screen space기준 수렴이 GIBS가 많이 빠름 

2차 BOUNCE LEAK은 둘다 비슷함 (코드를 통일하면 거의 차이가 없어질듯) 

성능은 대부분의 상황에서는 GIBS우위인것 같은데 잘 모르겠음. 근데 LOW FPS는 gist가 더 좋은듯 
TILE SURFEL이 뭉치는 상황에서는 GIST가 더 좋은데 나머지는 GIBS 우위인듯 
근데 GIBS에 SPECULAR가 아직 없다는 점을 반영해야함 

aa에 specular를 추가 안했는데 필요한지는 모르겠음. 

아직까지는 크게 거슬리지 않아서 그대로 둠. 
나중에 문제가 생기면 추가함.

GIST에서 성능 최적화 1순위가 
GIBS에서는 surfel 들 이었는데 
reconstruct 로 넘어간듯. tap이 3*3 에서 5x5로 늘면서 1pass당 0.4ms정도를 먹음. 그게 

나중에 GIBS와 GIST의 두 장점을 모두 통함하는 다른 방법이 생길수도? 

NEE + ray 최적화 
upload_data 최적화
blend_buffer를 삭제하고 main_buffer와 통합
skybox pass 통합?
### transparent specular 지원
### aa 리뉴얼, 그냥 opaque랑 transparent다 합치고, 시간축 blend하는게 나을듯?
### transparent mesh의 내부 산란 같은거 만들수 있을것 같은데?

## 망함. object_id 가 거짓말을 하고 있었음, 매 프레임 달라짐, surfel geo로 사용할수 없음
## object_render_data에 id를 넣는건?


## gltf load 
1. material shading id 추가  
2. submesh 지원 
    1. blas 생성시 각 geo_desc가 submesh가 된다. 
    1. rt_candidate_geometry_index 로 shader에서 얻는다
    1. primitive가 전역이 아니라 geo의 local이 된다. 
    1. meshlet build도 submesh당 build해야함, meshlet마다 submesh_idx가 있어야 할듯?
1. mesh { submesh_count, submesh[], omm[], alpha_texture[] }

3. double_face 가 mesh가 아니라 material에 붙어 있음... 
    1. 
4. alpha_mask 지원 필요함 
    1. omm 굽기 해야함.
    1. blas단계에서 구분이 필요해짐...
    1. alpha_mask_mesh 가 필요해짐 
    1. mesh + alpha_mask_texture + (파생된)omm_data 조합임. 



### submesh 지원 
기존 : 
tlas 마다 
		auto desc = D3D12_RAYTRACING_INSTANCE_DESC{
			.InstanceID							 = rt_instance_id_temp,
			.InstanceMask						 = to_idx(e::rt_mask_kind::transparent),
			.InstanceContributionToHitGroupIndex = 0,
			.Flags								 = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE,
			.AccelerationStructure				 = mesh_data_vec[mesh_id].h_blas->get_va(),
		};

Flags를 override했었음 
그런데 이게 blas의 geometry마다의 flag로 변경되어야함. 

blas는 여러개의 geometry의 집합임. 
blas를 구울때 각 geometry마다 flag를 줄 수 있음. 
그런데 tlas는 blas의 instance당 1개씩임. 
tlas는 전체 instance에 flag를 override할수 있지, 각 개별 geometry당 flag를 줄수 없음. 
그래서 transparent, opaque, omm 등은 tlas에서 설정할수 없음 

사실 import시에 다 분해해서 각 submesh마다 mesh 1개씩 만들면 구조변화 없이 갈수는 있음. 
대신 tlas 수가 늘어나고, 서로 다른 mesh를 같은 기준으로 lod를 관리해야 하는 시스템이 들어와야하고 
그럴바에는 그냥 처음부터 submesh쪽으로 가는게 맞음. 

대신 1mesh 1submesh모델을 유지하면 얻는게 있음 
사망시 fade같은 opaque <-> transparent 전환이 자유로움. 매 프레임 tlas를 재빌드하고 tlas단위에서 flag를 override하니깐
가능한 일임. 

그래서 일반적일때는 tlas flag에 none을 넣지만, 
obj 전체에 무언가를 적용하고 싶을때는 override할수 있게 해야하고 
그것에 맞춰서 raster pipeline 도 변경되어야함. 

그리고 obj 전체가 opaque -> transparent로 전환되면 gi가 한순간에 없어짐. 이를 어떻게 해결할지도 고민해야함. 


기존의 rt 에서 material_id 경로가 
geo -> submesh -> material로 indirection이 추가됨 

mask를 omm으로 처리하면 trace shader 코드는 transparent rt_proceed loop에서 submesh 데이터를 읽고 
mask인지 transparent인지만 구분해서 
transparent는 확률론적으로, 
mask면 material에서 alpha cutoff로 버리는식으로 처리해야함.
이렇게 되면 opaque + mask가 hardcap 이 되고, 그 아래 transparent가 주사위를 굴리고 시간축으로 blend 하게됨. 

어차피 non_opaque loop에서는 base_color sampling을 하고 
여기서 mask면 alpha만 mat.alpha_cutoff로 비교해서 버리거나 commit하면 됨. 
omm이 들어서도 shader code에서의 변경점은 거의 없음. 그냥 commit이 들 불리는거임 (UNKNOWN만 받음)

## alpha_mask 추가 
shader rt loop쪽은 거의 변경점이 없음. 
raster 쪽은 meshlet + render_data에 mask 전용이 생기고, 
opaque gbuffer 생성시에 다른 dispatch_mesh 단위에서 ps에서 alpha test후 discard해야함. 

mask 이면서 transparent인것도 구현 가능. 
mask이면서 어느 부분은 opauqe, 어느 부분은 transparent인것도 가능. 
gbuffer mask ps에서 alpha가 1이면 opaque, alpha 가 1 미만 alphacutoff 이상이면 transparent로 보내는것도 가능은함. 
다만 ps에서 uav가 비효율적이고 신경쓸게  있어서 
그냥 pass를 2개로 나누는것도 방법이긴함. 
두 방법중 무엇이 더 효율적일지는 몰?루? 아마 draw 2번이 단순하고 성능도 큰 차이 없어보일것 같긴함

일단은 omm 없이 추가하고, 추후에 omm을 같이 굽는 옵션을 제공하면 될듯 

mesh render override kind랑 
mesh render option flags랑 
fade 등을 묶어서 
mesh_render_option 이 추가로 필요해짐. 
fade는 rt_instance_render_data에 rt_mask_and_extra에 들어가야하고 
material_id를 uint16으로 내리고, object_render_data에 같이 넣어줘야함. 


texture마다 sampler 방식이 다를수 있음 
근데 어차피 개수가 정해져있어서 다 root sig에 박아두면 될듯 
아니면 bindless로 가거나 
그리고 material에서 texture마다 sampler type을 명시하면 될듯.

## material에 per texture sampler type추가 

## material_id를 uint32 에서 uint16 으로 변경

## asset에 asset version 추가
component와 다르게 asset의 경우 변경하면 기존 asset을 load할수가 없음. 
급한대로 asset file_header의 reserve에 asset_version을 넣었고 

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)

이 부분이 buf대신 buf + header 조합을 return 해야함. 
그리고 각 asset에서 version이 다르면 migrate하는 함수를 작성해야함.


## meshlet render data 구조를 재검토하고, cpu 에서 gpu driven으로 변경하기
지금 구조가 너무 안이쁨... 

## Model 의 추가. 
material개수가 1개로 고정일때는 문제가 없는데 
이젠 mesh 1개당 material이 다수가 됨. 
즉 material의 vector가 필요한데 그건 component로 못함. 
asset화 해야한다는건데 
그래서 등장한게 mesh + material array + etc 를 묶어서 
model asset으로 만들자 라는 아이디어. 

다만 model이 mesh bake option을 소유하게 되면, 
mesh 하나를 관리하던게 
mesh + option을 key로 관리해야함. 
그보다는 그냥 mesh 가 bake option을 가지고 있는게 더 관리하기 쉬울것 같음
variation이 많아지면, 그때가서 model을 변경하는게 나을듯.

그리고 나중에 model을 확장해서 lod를 지원하게 할수 있을듯.

struct entry<e::model> 
{
    h_mesh;
    // material_slot_count == h_mesh.submesh_count
    h_material* p_material;

}


## ui widget 정리하기 
자기 완결성이 있는것들은 return bool로 통일하고 
스타일 변경을 원하면 추가 desc를 받던 좀 정리를 해야할듯 
그리고 return bool을 모아서 뭔가 변경이 되었는지 쉽게 알수 있어야 하고 
그렇게 정리를 한 다음 editor component쪽에 비효율적으로 되어있는것을 정리해야함


## container들 self reference 살피기 
vector.emplace_back(vector[i]) 같은거 


## fade dither 가 segment edge를 유발하는 것, feature인가 bug인가

## gbuffer normal과 vis 를 분리?


## age::array 구현


## 엔진구조 
engine은 자기 완결적으로, 유저의 type이 필요 없음 
근데 editor만 유일하게 user renderer랑 ecs_game이 필요함 

engine - user_game_core - editor 의 구조로 가면 editor가 직접 user type을 알수 있고, user type이 변경될때마다 engine 재빌드 할 필요 없음
추후 ecs도 분리할수도 

editor game asset 과 
age_game asset 을 분리후 
각각 asset system과 통합하기


## ui 시스템 다듬기 
text_input 통일하기 
enter 막는 option 제공하기

## editor 다듬기 
default scene id 변경 가능하게 하기

## asset system 다듬기
cpu load, gpu load, 등등이 도메인마다 의미가 조금씩 다름 
설계 의도에 부합하긴 하지만 
editor 입장에서는 사용하기 힘들고 
switch문으로 해결하기에는 새로운 asset kind가 들어올때마다 
구현을 까먹을 여지가 있음 
그리고 material이나 model, 그리고 env_light의 경우 
가볍게 load하는 기능이 없음 
즉 cpu load도 여러 버전이 있을수 있음. 

asset dirty system 만들기

new asset과 import asset을 구분하기 
new asset은 기존 asset들을 활용해서 1개의 age_asset을 만드는것, 

import는 외부 file에서 1개 또는 그 이상의 age_asset을 생성하는것
기존 new texture가 import texture로 옮겨가야함. 

import는 editor 전용인가 아니면 asset system의 일종인가? 


model load -> child를 돌면서 is_loaded를 check한다. 
child가 바뀌었는데, handle만 있고 load가 아니라면 is_loaded가 false가 되면tj
file을 다시 읽어버린다 
그럼 model의 mat이 변화할때마다 renderer update할건가 
model이 render의 대상이 아닐수도 있다 (asset modal) 

model의 load를 
is_cpu_loaded와 is_gpu_loaded로 변경하자 

근데 is_cpu_loaded가 h_mesh의 is_cpu_loaded를 포함하는지 여부가 걸린다. 

맨 처음 어떤 방식이라도 load 가 되었음을 표기해야하나? 
is_file_loaded()? 

if_header_loaded()? 