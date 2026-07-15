__kernel void add(__global int* in1, __global int* in2, __global int* out) {
        size_t i = get_local_id(0);
        out[i] = in1[i] + in2[i];
}
