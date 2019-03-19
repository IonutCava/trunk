--Vertex

out flat int cull;

#include "vbInputData.vert"
#include "nodeBufferedInput.cmn"
#include "vegetationData.cmn"

void computeFoliageMovementTree(inout vec4 vertex, in float heightExtent) {
    float time = dvd_windDetails.w * dvd_time * 0.00025f; //to seconds
    float cosX = cos(vertex.x);
    float sinX = sin(vertex.x);
    float halfScale = 0.5f * heightExtent;
    vertex.x += (halfScale * cos(time) * cosX * sinX) * dvd_windDetails.x;
    vertex.z += (halfScale * sin(time) * cosX * sinX) * dvd_windDetails.z;
}

void main(void){

    computeDataNoClip();

    VegetationData data = TreeData(VAR.dvd_instanceID);

    float scale = data.positionAndScale.w;
    cull = (data.data.z > 2.1f) ? 1 : 0;

    if (data.data.z < 1.1f && dvd_Vertex.y * scale > 0.85f) {
        computeFoliageMovementTree(dvd_Vertex, data.data.w);
    }

    dvd_Vertex.xyz = rotate_vertex_position(dvd_Vertex.xyz, data.orientationQuad) * scale;
    

    VAR._vertexW = (dvd_Vertex + vec4(data.positionAndScale.xyz, 0.0f));

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
#if !defined(DEPTH_PASS)
    VAR._normalWV = normalize(mat3(dvd_ViewMatrix) * -dvd_Normal);
#endif

#if !defined(SHADOW_PASS)
    setClipPlanes(VAR._vertexW);
#endif

    //Compute the final vert position
    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;

}

-- Geometry

in flat int cull[];

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

void main()
{
    if (cull[0] == 0) {
        for (int i = 0; i < gl_in.length(); ++i)
        {
            PassData(i);
            gl_Position = gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}

-- Fragment

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

layout(early_fragment_tests) in;

#include "BRDF.frag"
#include "output.frag"

void main (void) {
    vec3 normal = getNormal();

    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    vec4 albedo = getAlbedo(colourMatrix);
#if defined(USE_ALPHA_DISCARD)
    if (albedo.a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

    writeOutput(getPixelColour(albedo, colourMatrix, normal), packNormal(normal));
}


--Fragment.PrePass.AlphaDiscard
#include "materialData.frag"

void main() {
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;

    if (getAlbedo(colourMatrix).a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
}

--Fragment.Shadow
#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

out vec2 _colourOut;

#if defined(HAS_TRANSPARENCY)
#include "materialData.frag"
#else
#include "nodeBufferedInput.cmn"
#endif
#include "utility.frag"

#include "vsm.frag"

void main() {
#if defined(HAS_TRANSPARENCY)
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    if (getAlbedo(colourMatrix).a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

    _colourOut = computeMoments();
}