out float gl_ClipDistance[MAX_CLIP_PLANES];

uniform vec4 dvd_clip_plane[MAX_CLIP_PLANES];


void setClipPlanes(in vec4 worldSpaceVertex) {
#if MAX_CLIP_PLANES > 0
    gl_ClipDistance[0] = dot(worldSpaceVertex, dvd_clip_plane[0]);
#endif
#if MAX_CLIP_PLANES > 1
    gl_ClipDistance[1] = dot(worldSpaceVertex, dvd_clip_plane[1]);
#endif
#if MAX_CLIP_PLANES > 2
    gl_ClipDistance[2] = dot(worldSpaceVertex, dvd_clip_plane[2]);
#endif
#if MAX_CLIP_PLANES > 3
    gl_ClipDistance[3] = dot(worldSpaceVertex, dvd_clip_plane[3]);
#endif
#if MAX_CLIP_PLANES > 4
    gl_ClipDistance[4] = dot(worldSpaceVertex, dvd_clip_plane[4]);
#endif
#if MAX_CLIP_PLANES > 5
    gl_ClipDistance[5] = dot(worldSpaceVertex, dvd_clip_plane[5]);
#endif
}