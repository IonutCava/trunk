-- Vertex

void main(void)
{
    vec2 uv = vec2(0,0);
    if((gl_VertexID & 1) != 0)uv.x = 1;
    if((gl_VertexID & 2) != 0)uv.y = 1;

    VAR._texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0,1);
}

-- Fragment

/****************************************************************************/
/* Copyright (c) 2011, Ola Olsson, Ulf Assarsson
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/****************************************************************************/
uniform mat4 dvd_ProjectionMatrixInverse;
uniform sampler2D depthTex;

out vec2 resultMinMax;

vec3 unProject(vec2 fragmentPos, float fragmentDepth)
{
  vec4 pt = dvd_ProjectionMatrixInverse * 
            vec4(fragmentPos.x * 2.0 - 1.0, 
                 fragmentPos.y * 2.0 - 1.0, 
                 2.0 * fragmentDepth - 1.0, 
                 1.0);

  return vec3(pt.x, pt.y, pt.z) / pt.w;
}


vec3 fetchPosition(vec2 p, float d)
{
  vec2 fragmentPos = vec2(p.x * dvd_invScreenDimensions.x, p.y * dvd_invScreenDimensions.y);
  return unProject(fragmentPos, d);
}

void main()
{
    vec2 minMax = vec2(1.0f, -1.0f);
    ivec2 offset = ivec2(gl_FragCoord.xy) * ivec2(LIGHT_GRID_TILE_DIM_X, LIGHT_GRID_TILE_DIM_Y);
    ivec2 end = min(dvd_ViewPort.zw, offset + ivec2(LIGHT_GRID_TILE_DIM_X, LIGHT_GRID_TILE_DIM_Y));

    // Note: with large tiles and many samples this shader makes very poor use of 
    // graphics hardware paralellism. A few shader threads will perform a lot of sequential 
    // work, and on top of this, access memory in a fairly nasty pattern. 
    for (int j = offset.y; j < end.y; ++j)
    {
        for (int i = offset.x; i < end.x; ++i)
        {
          float d = texelFetch(depthTex, ivec2(i,j), 0).x;
            if (d < 1.0)
            {
                minMax.x = min(minMax.x, d);
                minMax.y = max(minMax.y, d);
            }
        }
    }

    // somewhat roundabout way to get to view space depth (note: this is NOT the dodgy part of this process 
    // see above... :).
    minMax = vec2(fetchPosition(vec2(0.0, 0.0), minMax.x).z, fetchPosition(vec2(0.0, 0.0), minMax.y).z);
    resultMinMax = minMax;
}
