const int order = 1;

__kernel void sobel(__read_only image2d_t srcImg, __write_only image2d_t dstImg, sampler_t sampler, int width, int height))
{
	gid = get_global_id(0);	    
}

