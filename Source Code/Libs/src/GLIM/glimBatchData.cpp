/*
** GLIM - OpenGL Immediate Mode
** Copyright Jan Krassnigg (Jan@Krassnigg.de)
** For more details, see the included Readme.txt.
*/

#include "glimBatchData.h"
#include "glimInterface.h"

#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"

#define BUFFER_OFFSET(i) ((char *)nullptr + (i))

namespace NS_GLIM
{
    GlimArrayData::GlimArrayData ()
    {
        Reset ();	
    }

    void GlimArrayData::Reset (void)
    {
        m_DataType = GLIM_NODATA;
        m_ArrayData.clear ();
        m_ArrayData.reserve (256);

        m_CurrentValue[0].Int = 0;
        m_CurrentValue[1].Int = 0;
        m_CurrentValue[2].Int = 0;
        m_CurrentValue[3].Int = 0;
    }

    void GlimArrayData::PushElement (void)
    {
        switch (m_DataType)
        {
        // 4 byte data
        case GLIM_1I:
        case GLIM_1F:
        case GLIM_4UB:
            m_ArrayData.push_back (m_CurrentValue[0]);
            return;

        // 8 byte data
        case GLIM_2I:
        case GLIM_2F:
            m_ArrayData.push_back (m_CurrentValue[0]);
            m_ArrayData.push_back (m_CurrentValue[1]);
            return;

        // 12 byte data
        case GLIM_3I:
        case GLIM_3F:
            m_ArrayData.push_back (m_CurrentValue[0]);
            m_ArrayData.push_back (m_CurrentValue[1]);
            m_ArrayData.push_back (m_CurrentValue[2]);
            return;

        // 16 byte data
        case GLIM_4I:
        case GLIM_4F:
            m_ArrayData.push_back (m_CurrentValue[0]);
            m_ArrayData.push_back (m_CurrentValue[1]);
            m_ArrayData.push_back (m_CurrentValue[2]);
            m_ArrayData.push_back (m_CurrentValue[3]);
            return;

        default:
            // throws an exception
            GLIM_CHECK (false, "GlimArrayData::PushElement: Data-Type is invalid.");
            return;
        }
    }

    void glimBatchData::Upload (unsigned int uiCurrentProgram)
    {
#ifdef AE_RENDERAPI_OPENGL
        UploadOGL (uiCurrentProgram);
#else
        UploadD3D11 ();
#endif
    }

    void glimBatchData::Bind (unsigned int uiCurrentProgram)
    {
#ifdef AE_RENDERAPI_OPENGL
        BindOGL (uiCurrentProgram);
#else
        BindD3D11 (uiCurrentProgram);
#endif
    }

    void glimBatchData::Unbind (void)
    {
#ifdef AE_RENDERAPI_OPENGL
        UnbindOGL ();
#else
        UnbindD3D11 ();
#endif
    }

    glimBatchData::glimBatchData ()
    {
        m_bUploadedToGPU = false;
        m_bCreatedVBOs = false;

#ifdef AE_RENDERAPI_D3D11
        m_pVertexBuffer = nullptr;
        m_pIndexBuffer_Points = nullptr;
        m_pIndexBuffer_Lines = nullptr;
        m_pIndexBuffer_Triangles = nullptr;
        m_pIndexBuffer_Wireframe = nullptr;
#endif

        Reset ();
    }

    glimBatchData::~glimBatchData ()
    {
        Reset ();

#ifdef AE_RENDERAPI_OPENGL
        if (m_bCreatedVBOs)
        {
            m_bCreatedVBOs = false;
            glDeleteVertexArrays(1, &m_VertexArrayObjectID);
            glDeleteBuffers (1, &m_uiVertexBufferID);
            glDeleteBuffers (1, &m_uiElementBufferID_Points);
            glDeleteBuffers (1, &m_uiElementBufferID_Lines);
            glDeleteBuffers (1, &m_uiElementBufferID_Triangles);
            glDeleteBuffers (1, &m_uiElementBufferID_Wireframe);
        }

#endif
    }

    void glimBatchData::Reset (void)
    {
        m_State = STATE_EMPTY;

        m_Attributes.clear ();
        m_PositionData.clear ();
        m_PositionData.reserve (64 * 3);

        m_IndexBuffer_Points.clear ();
        m_IndexBuffer_Points.reserve (16);
        m_IndexBuffer_Lines.clear ();
        m_IndexBuffer_Lines.reserve (32);
        m_IndexBuffer_Triangles.clear ();
        m_IndexBuffer_Triangles.reserve (128);
        m_IndexBuffer_Wireframe.clear ();
        m_IndexBuffer_Wireframe.reserve (128);

        m_bUploadedToGPU = false;

        m_fMinX = 99999999.0f;
        m_fMaxX =-99999999.0f;
        m_fMinY = 99999999.0f;
        m_fMaxY =-99999999.0f;
        m_fMinZ = 99999999.0f;
        m_fMaxZ =-99999999.0f;

#ifdef AE_RENDERAPI_D3D11
        if (m_pVertexBuffer)
        {
            GLIM_Interface::s_ReleaseResourceCallback (m_pVertexBuffer);
            //m_pVertexBuffer->Release ();
            m_pVertexBuffer = nullptr;
        }
        if (m_pIndexBuffer_Points)
        {
            GLIM_Interface::s_ReleaseResourceCallback (m_pIndexBuffer_Points);
            //m_pIndexBuffer_Points->Release ();
            m_pIndexBuffer_Points = nullptr;
        }
        if (m_pIndexBuffer_Lines)
        {
            GLIM_Interface::s_ReleaseResourceCallback (m_pIndexBuffer_Lines);
            //m_pIndexBuffer_Lines->Release ();
            m_pIndexBuffer_Lines = nullptr;
        }
        if (m_pIndexBuffer_Triangles)
        {
            GLIM_Interface::s_ReleaseResourceCallback (m_pIndexBuffer_Triangles);
            //m_pIndexBuffer_Triangles->Release ();
            m_pIndexBuffer_Triangles = nullptr;
        }
        if (m_pIndexBuffer_Wireframe)
        {
            GLIM_Interface::s_ReleaseResourceCallback (m_pIndexBuffer_Wireframe);
            //m_pIndexBuffer_Wireframe->Release ();
            m_pIndexBuffer_Wireframe = nullptr;
        }

        m_Signature.clear ();
#endif
    }

    unsigned int glimBatchData::AddVertex (float x, float y, float z)
    {
        if (x < m_fMinX)
            m_fMinX = x;
        if (x > m_fMaxX)
            m_fMaxX = x;

        if (y < m_fMinY)
            m_fMinY = y;
        if (y > m_fMaxY)
            m_fMaxY = y;

        if (z < m_fMinZ)
            m_fMinZ = z;
        if (z > m_fMaxZ)
            m_fMaxZ = z;

        m_PositionData.push_back (x);
        m_PositionData.push_back (y);
        m_PositionData.push_back (z);

        std::map<std::string, GlimArrayData>::iterator it, itend;
        itend = m_Attributes.end ();

        for (it = m_Attributes.begin (); it != itend; ++it)
            it->second.PushElement ();

        return ((unsigned int) (m_PositionData.size () / 3) - 1);
    }

#ifdef AE_RENDERAPI_D3D11
    void glimBatchData::GenerateSignature (void)
    {
        m_Signature.clear ();
        m_Signature.reserve (1 + m_Attributes.size ());

        D3D11_INPUT_ELEMENT_DESC sig;
        sig.InputSlot = 0;
        sig.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        sig.InstanceDataStepRate = 0;
        sig.SemanticIndex = 0;
        sig.SemanticName = GLIM_Interface::s_VertexPosSemanticName;
        sig.AlignedByteOffset = 0;
        sig.Format = DXGI_FORMAT_R32G32B32_FLOAT;

        m_Signature.push_back (sig);

        std::map<std::string, GlimArrayData>::const_iterator it, itend;
        itend = m_Attributes.end ();

        unsigned int uiOffset = sizeof (float) * 3;

        int iSlot = 1;
        for (it = m_Attributes.begin (); it != itend; ++it, ++iSlot)
        {
            D3D11_INPUT_ELEMENT_DESC sig;
            sig.InputSlot = iSlot;
            sig.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            sig.InstanceDataStepRate = 0;
            sig.SemanticIndex = 0;
            sig.SemanticName = it->first.c_str ();
            sig.AlignedByteOffset = 0;//uiOffset;

            switch (it->second.m_DataType)
            {
            // 4 byte data
            case GLIM_1I:
                uiOffset += sizeof (int) * 1;
                sig.Format = DXGI_FORMAT_R32_SINT;
                break;
            case GLIM_1F:
                uiOffset += sizeof (float) * 1;
                sig.Format = DXGI_FORMAT_R32_FLOAT;
                break;
            case GLIM_4UB:
                uiOffset += sizeof (int) * 1;
                sig.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;

            // 8 byte data
            case GLIM_2I:
                uiOffset += sizeof (int) * 2;
                sig.Format = DXGI_FORMAT_R32G32_SINT;
                break;
            case GLIM_2F:
                uiOffset += sizeof (float) * 2;
                sig.Format = DXGI_FORMAT_R32G32_FLOAT;
                break;

            // 12 byte data
            case GLIM_3I:
                uiOffset += sizeof (int) * 3;
                sig.Format = DXGI_FORMAT_R32G32B32_SINT;
                break;
            case GLIM_3F:
                uiOffset += sizeof (float) * 3;
                sig.Format = DXGI_FORMAT_R32G32B32_FLOAT;
                break;

            // 16 byte data
            case GLIM_4I:
                uiOffset += sizeof (int) * 4;
                sig.Format = DXGI_FORMAT_R32G32B32A32_SINT;
                break;
            case GLIM_4F:
                uiOffset += sizeof (float) * 4;
                sig.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;

            default:
                // throws an exception
                GLIM_CHECK (false, "glimBatchData::GenerateSignature: Data-Type is invalid.");
                break;
            }

            m_Signature.push_back (sig);
        }
    }
#endif

    unsigned int glimBatchData::getVertexDataSize (void) const
    {
        unsigned int uiVertexDataSize = sizeof (float) * 3;

        std::map<std::string, GlimArrayData>::const_iterator it, itend;
        itend = m_Attributes.end ();

        for (it = m_Attributes.begin (); it != itend; ++it)
        {
            switch (it->second.m_DataType)
            {
            // 4 byte data
            case GLIM_1I:
            case GLIM_1F:
            case GLIM_4UB:
                uiVertexDataSize += sizeof (int) * 1;
                break;

            // 8 byte data
            case GLIM_2I:
            case GLIM_2F:
                uiVertexDataSize += sizeof (int) * 2;
                break;

            // 12 byte data
            case GLIM_3I:
            case GLIM_3F:
                uiVertexDataSize += sizeof (int) * 3;
                break;

            // 16 byte data
            case GLIM_4I:
            case GLIM_4F:
                uiVertexDataSize += sizeof (int) * 4;
                break;

            default:
                // throws an exception
                GLIM_CHECK (false, "glimBatchData::Upload: Data-Type is invalid.");
                break;
            }
        }

        return (uiVertexDataSize);
    }

#ifdef AE_RENDERAPI_OPENGL

    int glimBatchData::getVertexAttribBindPoint (unsigned int uiCurrentProgram, const char* szAttribName) const
    {
        int iLocation = glGetAttribLocation (uiCurrentProgram, szAttribName);

        return (iLocation);
    }

    void glimBatchData::UnbindOGL (void)
    {
        GL_API::setActiveVAO(0);
        /*GL_API::setActiveBuffer(GL_ARRAY_BUFFER, 0);
        GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        for (int i = 0; i < 16; ++i)
            glDisableVertexAttribArray (i);*/
    }

    void glimBatchData::BindOGL (unsigned int uiCurrentProgram)
    {
        if (!m_bUploadedToGPU)
            return;

        GL_API::setActiveVAO(m_VertexArrayObjectID);
        GL_API::setActiveBuffer(GL_ARRAY_BUFFER, m_uiVertexBufferID);
        std::map<std::string, GlimArrayData>::iterator it, itend;
        itend = m_Attributes.end ();

        for (it = m_Attributes.begin (); it != itend; ++it)
        {
            const int iAttributeArray = getVertexAttribBindPoint (uiCurrentProgram, it->first.c_str ());

            if (iAttributeArray < 0)
                continue;

            glEnableVertexAttribArray (iAttributeArray);

            switch (it->second.m_DataType)
            {
            case GLIM_1F:
                glVertexAttribPointer (iAttributeArray, 1, GL_FLOAT, false, 0, BUFFER_OFFSET (it->second.m_uiBufferOffset));
                break;
            case GLIM_2F:
                glVertexAttribPointer (iAttributeArray, 2, GL_FLOAT, false, 0, BUFFER_OFFSET (it->second.m_uiBufferOffset));
                break;
            case GLIM_3F:
                glVertexAttribPointer (iAttributeArray, 3, GL_FLOAT, false, 0, BUFFER_OFFSET (it->second.m_uiBufferOffset));
                break;
            case GLIM_4F:
                glVertexAttribPointer (iAttributeArray, 4, GL_FLOAT, false, 0, BUFFER_OFFSET (it->second.m_uiBufferOffset));
                break;
            case GLIM_1I:
                glVertexAttribIPointer (iAttributeArray, 1, GL_INT, 0, BUFFER_OFFSET (it->second.m_uiBufferOffset));
                break;
            case GLIM_2I:
                glVertexAttribIPointer (iAttributeArray, 2, GL_INT, 0, BUFFER_OFFSET (it->second.m_uiBufferOffset));
                break;
            case GLIM_3I:
                glVertexAttribIPointer (iAttributeArray, 3, GL_INT, 0, BUFFER_OFFSET (it->second.m_uiBufferOffset));
                break;
            case GLIM_4I:
                glVertexAttribIPointer (iAttributeArray, 4, GL_INT, 0, BUFFER_OFFSET (it->second.m_uiBufferOffset));
                break;
            case GLIM_4UB:
                glVertexAttribPointer (iAttributeArray, 4, GL_UNSIGNED_BYTE, true, 0, BUFFER_OFFSET (it->second.m_uiBufferOffset));
                break;
            }
        }

        // set the pointer for position last
        glEnableVertexAttribArray (m_VertAttribLocation);
        glVertexAttribPointer (m_VertAttribLocation, 3, GL_FLOAT, false, 0, BUFFER_OFFSET (0));
        
    }

    void glimBatchData::UploadOGL (unsigned int uiCurrentProgram)
    {
        if (m_bUploadedToGPU)
            return;

        if (m_PositionData.empty ())
            return;

        m_bUploadedToGPU = true;
        
        const unsigned int uiVertexDataSize = (unsigned int) getVertexDataSize ();
        const unsigned int uiVertices = (unsigned int) (m_PositionData.size ()) / 3;

        if (!m_bCreatedVBOs)
        {
            m_bCreatedVBOs = true;
            glGenVertexArrays(1, &m_VertexArrayObjectID);
            glGenBuffers (1, &m_uiVertexBufferID);
            glGenBuffers (1, &m_uiElementBufferID_Points);
            glGenBuffers (1, &m_uiElementBufferID_Lines);
            glGenBuffers (1, &m_uiElementBufferID_Triangles);
            glGenBuffers (1, &m_uiElementBufferID_Wireframe);
        }
            
        GL_API::setActiveBuffer(GL_ARRAY_BUFFER, m_uiVertexBufferID);
        glBufferData (GL_ARRAY_BUFFER, uiVertices * uiVertexDataSize, NULL, GL_STREAM_DRAW);

        // first upload the position data
        unsigned int uiOffset = uiVertices * sizeof (float) * 3;

        // offset is zero, but can use uiOffset as size
        glBufferSubData (GL_ARRAY_BUFFER, 0, uiOffset, &m_PositionData[0]);
        // the buffer in RAM can be cleared now
        m_PositionData.clear ();
        
        // now upload each attribute array one after another

        std::map<std::string, GlimArrayData>::iterator it, itend;
        itend = m_Attributes.end ();

        for (it = m_Attributes.begin (); it != itend; ++it)
        {
            const unsigned int uiAttributeSize = (unsigned int) (it->second.m_ArrayData.size ()) * 4; // already includes the number of vertices

            // upload the data
            glBufferSubData (GL_ARRAY_BUFFER, uiOffset, uiAttributeSize, &it->second.m_ArrayData[0].Float);

            // free the temporary buffer in RAM, the data is now in a better place (the GPU)
            it->second.m_ArrayData.clear ();

            it->second.m_uiBufferStride = uiAttributeSize / uiVertices;

            // store the buffer offset for later use
            it->second.m_uiBufferOffset = uiOffset;

            // increase the buffer offset
            uiOffset += uiAttributeSize;
        }

        m_uiPointElements = 0;
        m_uiLineElements = 0;
        m_uiTriangleElements = 0;
        m_uiWireframeElements = 0;

        // upload the index buffer for the points
        GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiElementBufferID_Points);
        if (!m_IndexBuffer_Points.empty ())
        {
            m_uiPointElements = (unsigned int) m_IndexBuffer_Points.size ();
            glBufferData (GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer_Points.size () * 4, &m_IndexBuffer_Points[0], GL_STATIC_DRAW);
            m_IndexBuffer_Points.clear ();
        }
            
        // upload the index buffer for the lines
        GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiElementBufferID_Lines);
        if (!m_IndexBuffer_Lines.empty ())
        {
            m_uiLineElements = (unsigned int) m_IndexBuffer_Lines.size ();
            glBufferData (GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer_Lines.size () * 4, &m_IndexBuffer_Lines[0], GL_STATIC_DRAW);
            m_IndexBuffer_Lines.clear ();
        }


        // upload the index buffer for the triangles
        GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiElementBufferID_Triangles);
        if (!m_IndexBuffer_Triangles.empty ())
        {
            m_uiTriangleElements = (unsigned int) m_IndexBuffer_Triangles.size ();
            glBufferData (GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer_Triangles.size () * 4, &m_IndexBuffer_Triangles[0], GL_STATIC_DRAW);
            m_IndexBuffer_Triangles.clear ();
        }
    }
#endif
}


