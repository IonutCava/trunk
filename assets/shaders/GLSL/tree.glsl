--Vertex

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

    const VegetationData data = TreeData(VAR.dvd_instanceID);

    float scale = data.positionAndScale.w;

    const float LoDValue = data.data.z;
    if (LoDValue > 2.1f) {
        scale = 0.0f;
        //gl_CullDistance[0] = -0.01f;
    }

    if (LoDValue < 1.1f && dvd_Vertex.y * scale > 0.85f) {
        //computeFoliageMovementTree(dvd_Vertex, data.data.w);
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

-- Fragment

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

layout(early_fragment_tests) in;

#include "BRDF.frag"
#include "output.frag"

void main (void) {
    updateTexCoord();

    vec3 normal = getNormal();

    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    vec4 albedo = getAlbedo(colourMatrix);
#if defined(USE_ALPHA_DISCARD)
    if (albedo.a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

    writeOutput(getPixelColour(albedo, colourMatrix, normal));
}


--Fragment.PrePass

#include "BRDF.frag"
#include "materialData.frag"
#include "velocityCalc.frag"

layout(location = 1) out vec4 _normalAndVelocityOut;

void main() {
    updateTexCoord();

    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;

#if defined(USE_ALPHA_DISCARD)
    if (getAlbedo(colourMatrix).a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

    _normalAndVelocityOut.rg = packNormal(normalize(getNormal()));
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
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