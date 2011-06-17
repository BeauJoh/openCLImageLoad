const int order = 1;

__kernel void sobel(__global uint8_t* data, __global uint8_t* out)
{
	gid = get_global_id(0);	
	out[gid] = data[gid];
    
}

