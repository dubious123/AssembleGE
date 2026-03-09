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