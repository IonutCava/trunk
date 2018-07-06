-- Vertex

void main(void)
{

    gl_Position = vec4(inVertexData,1.0);
}


-- Fragment

out vec4 _colorOut;

void main(void){
    _colorOut = vec4(1.0);
}
