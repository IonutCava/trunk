-- Vertex

uniform vec3 ObjectExtent;
uniform mat4 dvd_WorldViewProjectionMatrix;

layout(location = 11) in float scale;

out vec4 OrigPosition;
flat out float OrigScale;
flat out int objectVisible;

void main(void) {
    
    OrigPosition = vec4(inVertexData, 1.0);
    OrigScale = scale;

    // create the bounding box of the object 
    vec4 BoundingBox[8];
    BoundingBox[0] = dvd_WorldViewProjectionMatrix * (OrigPosition + vec4( ObjectExtent.x, ObjectExtent.y, ObjectExtent.z, 1.0));
    BoundingBox[1] = dvd_WorldViewProjectionMatrix * (OrigPosition + vec4(-ObjectExtent.x, ObjectExtent.y, ObjectExtent.z, 1.0));
    BoundingBox[2] = dvd_WorldViewProjectionMatrix * (OrigPosition + vec4( ObjectExtent.x,-ObjectExtent.y, ObjectExtent.z, 1.0));
    BoundingBox[3] = dvd_WorldViewProjectionMatrix * (OrigPosition + vec4(-ObjectExtent.x,-ObjectExtent.y, ObjectExtent.z, 1.0));
    BoundingBox[4] = dvd_WorldViewProjectionMatrix * (OrigPosition + vec4( ObjectExtent.x, ObjectExtent.y,-ObjectExtent.z, 1.0));
    BoundingBox[5] = dvd_WorldViewProjectionMatrix * (OrigPosition + vec4(-ObjectExtent.x, ObjectExtent.y,-ObjectExtent.z, 1.0));
    BoundingBox[6] = dvd_WorldViewProjectionMatrix * (OrigPosition + vec4( ObjectExtent.x,-ObjectExtent.y,-ObjectExtent.z, 1.0));
    BoundingBox[7] = dvd_WorldViewProjectionMatrix * (OrigPosition + vec4(-ObjectExtent.x,-ObjectExtent.y,-ObjectExtent.z, 1.0));

    // check how the bounding box resides regarding to the view frustum 
    int outOfBound[6] = int[6](0, 0, 0, 0, 0, 0);
    float limit = 0;
    for (int i = 0; i<8; i++)
    {
        limit = BoundingBox[i].w;
        if (BoundingBox[i].x >  limit) outOfBound[0]++;
        if (BoundingBox[i].x < -limit) outOfBound[1]++;
        if (BoundingBox[i].y >  limit) outOfBound[2]++;
        if (BoundingBox[i].y < -limit) outOfBound[3]++;
        if (BoundingBox[i].z >  limit) outOfBound[4]++;
        if (BoundingBox[i].z < -limit) outOfBound[5]++;
    }

    for (int i = 0; i < 6; i++)
        if (outOfBound[i] == 8) {
            objectVisible = 0;
            return;
        }
    
    objectVisible = 1;
}

-- Geometry

layout(points) in;
layout(points, max_vertices = 1) out;

in vec4 OrigPosition[1];
flat in float OrigScale[1];
flat in int objectVisible[1];

out vec3 outVertexData;
out float outData1;

void main() {

    // only emit primitive if the object is visible 
    if (objectVisible[0] == 1)
    {
        outVertexData = OrigPosition[0].xyz;
        outData1 = OrigScale[0];
        EmitVertex();
        EndPrimitive();
    }
}
