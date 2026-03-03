# C++ TODO (AGE)

## On MSVC Update (Toolset / STL / Compiler behavior)
### 1. custom static assert message 
- move run_sys static_assert to pipe, adaptor, on_ctx ... 
### 2. variadic indexing
- re-impl meta::variadic_at, index, ...

## Compressed pack
### 1. impl std::get

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

## handle pdb

## clean up data_structure, write data_structure benchmark and test

## better macro, (a,b,c) -> (a)(b)(c) to reduce macro parameter size

## do pgo

## do execute indirect

## offset allocator

## material system ( programmable render pipeline stage )

## imple raw input

## fix cone culling (really), maybe sphere mesh generation code bug

## lights!!
### 5. area light 


### Light Culling Optimization (현재 조사 중)
- **Z-binning + 2D tile bitmask (CoD IW 방식)**: 3D 클러스터(O(X×Y×Z)) 대신 XY/Z를 분리하여 O(X×Y + Z)로 줄임
  - 라이트를 view-space Z 기준 정렬, Z bin당 {start, end} 인덱스
  - 2D 타일당 라이트 비트마스크, PS에서 Z bin ∩ tile mask로 최종 라이트 결정
  - 참고: Activision SIGGRAPH 2017, Granite engine blog

### AS Culling 중복 제거
- depth prepass AS와 opaque AS가 동일한 frustum/cone culling을 2회 수행
- depth prepass AS에서 visible mask를 버퍼에 저장, opaque AS에서 재사용
- 현재 AS 병목은 아님 (PIX 기준 수십μs), 우선순위 낮음