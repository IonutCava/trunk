#ifndef _IN_OUT_GEOM_
#define _IN_OUT_GEOM_

void passVertex(int index) {
    g_out.dvd_drawID = VAR[index].dvd_drawID;
    g_out._vertexW  = VAR[index]._vertexW;
    g_out._vertexWV = VAR[index]._vertexWV;
    g_out._normalWV = VAR[index]._normalWV;
    g_out._tangentWV = VAR[index]._tangentWV;
    g_out._bitangentWV = VAR[index]._bitangentWV;
    g_out._texCoord = VAR[index]._texCoord;
}

#endif //_IN_OUT_GEOM_