
uniform int dvd_clip_plane_count;
uniform vec4 dvd_clip_plane[MAX_CLIP_PLANES];

void setClipPlanes(in vec4 worldVertex) {
#if MAX_CLIP_PLANES > 0
    if (dvd_clip_plane_count == 0) return;
    gl_ClipDistance[0] = dot(worldVertex, dvd_clip_plane[0]);
#	if MAX_CLIP_PLANES > 1
        if (dvd_clip_plane_count == 1) return;
        gl_ClipDistance[1] = dot(worldVertex, dvd_clip_plane[1]);
#		if MAX_CLIP_PLANES > 2
            if (dvd_clip_plane_count == 2) return;
            gl_ClipDistance[2] = dot(worldVertex, dvd_clip_plane[2]);
#			if MAX_CLIP_PLANES > 3
                if (dvd_clip_plane_count == 3) return;
                gl_ClipDistance[3] = dot(worldVertex, dvd_clip_plane[3]);
#				if MAX_CLIP_PLANES > 4
                    if (dvd_clip_plane_count == 4) return;
                    gl_ClipDistance[4] = dot(worldVertex, dvd_clip_plane[4]);
#					if MAX_CLIP_PLANES > 5	
                        if (dvd_clip_plane_count == 5) return;
                        gl_ClipDistance[5] = dot(worldVertex, dvd_clip_plane[5]);
#					endif //5
#				endif //4
#			endif //3
#		endif //2
#	endif //1
#endif //0
}