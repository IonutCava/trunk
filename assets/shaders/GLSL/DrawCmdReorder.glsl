--Compute

layout(local_size_x = 64) in;

layout(location = 0) uniform uint dvd_numEntities;

void main()
{
    uint ident = gl_GlobalInvocationID.x;
}
