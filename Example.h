//
// Created by Nikita on 05.05.2021.
//

#ifndef EVOVULKAN_EXAMPLE_H
#define EVOVULKAN_EXAMPLE_H

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <GLFW/glfw3.h>

#include <EvoVulkan/Types/Texture.h>

#include <stbi.h>

#include <EvoVulkan/VulkanKernel.h>
#include <EvoVulkan/Complexes/Shader.h>
#include <EvoVulkan/Complexes/Mesh.h>

using namespace EvoVulkan;

struct UniformBuffer {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
};

struct Vertex {
    float position[3];
    float uv[2];
};

const float skyboxVertices[36 * 3] = {
        // positions
        -10.0f,  10.0f, -10.0f,
        -10.0f, -10.0f, -10.0f,
        10.0f, -10.0f, -10.0f,
        10.0f, -10.0f, -10.0f,
        10.0f,  10.0f, -10.0f,
        -10.0f,  10.0f, -10.0f,

        -10.0f, -10.0f,  10.0f,
        -10.0f, -10.0f, -10.0f,
        -10.0f,  10.0f, -10.0f,
        -10.0f,  10.0f, -10.0f,
        -10.0f,  10.0f,  10.0f,
        -10.0f, -10.0f,  10.0f,

        10.0f, -10.0f, -10.0f,
        10.0f, -10.0f,  10.0f,
        10.0f,  10.0f,  10.0f,
        10.0f,  10.0f,  10.0f,
        10.0f,  10.0f, -10.0f,
        10.0f, -10.0f, -10.0f,

        -10.0f, -10.0f,  10.0f,
        -10.0f,  10.0f,  10.0f,
        10.0f,  10.0f,  10.0f,
        10.0f,  10.0f,  10.0f,
        10.0f, -10.0f,  10.0f,
        -10.0f, -10.0f,  10.0f,

        -10.0f,  10.0f, -10.0f,
        10.0f,  10.0f, -10.0f,
        10.0f,  10.0f,  10.0f,
        10.0f,  10.0f,  10.0f,
        -10.0f,  10.0f,  10.0f,
        -10.0f,  10.0f, -10.0f,

        -10.0f, -10.0f, -10.0f,
        -10.0f, -10.0f,  10.0f,
        10.0f, -10.0f, -10.0f,
        10.0f, -10.0f, -10.0f,
        -10.0f, -10.0f,  10.0f,
        10.0f, -10.0f,  10.0f
};

struct mesh {
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    Types::Buffer*  m_uniformBuffer = nullptr;
    Types::Buffer*  m_vertexBuffer  = nullptr;
    Types::Buffer*  m_indexBuffer   = nullptr;
    UniformBuffer   m_ubo           = {};
    uint64_t        m_countIndices  = 0;

    __forceinline void Draw(const VkCommandBuffer& cmd, const VkPipelineLayout& layout) const {
        VkDeviceSize offsets[1] = {0};

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &m_descriptorSet, 0, NULL);
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer->m_buffer, offsets);
        vkCmdBindIndexBuffer(cmd, m_indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd, m_countIndices, 1, 0, 0, 0);
    }
};

class VulkanExample : public Core::VulkanKernel {
private:
    Complexes::Shader*          m_shader              = nullptr;
    Types::Texture*             m_texture             = nullptr;

    mesh meshes[3];
    mesh skybox;
public:
    void Render() override {
        if (this->PrepareFrame() == Core::FrameResult::OutOfDate)
            this->m_hasErrors = !this->ResizeWindow();

        // Command buffer to be submitted to the queue
        m_submitInfo.commandBufferCount = 1;
        m_submitInfo.pCommandBuffers    = &m_drawCmdBuffs[m_currentBuffer];

        // Submit to queue
        auto result = vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &m_submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            VK_ERROR("renderFunction() : failed to queue submit!");
            return;
        }

        if (this->SubmitFrame() == Core::FrameResult::OutOfDate)
            this->m_hasErrors = !this->ResizeWindow();
    }

    void UpdateUBO() {
        glm::mat4 projectionMatrix = glm::perspective(
                glm::radians(
                        1000.f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
                4.0f /
                3.0f,       // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
                0.1f,              // Near clipping plane. Keep as big as possible, or you'll get precision issues.
                100.0f             // Far clipping plane. Keep as little as possible.
        );

        /*glm::mat4 view = glm::lookAt(
                glm::vec3(4, 3, 3), // Camera is at (4,3,3), in World Space
                glm::vec3(0, 0, 0), // and looks at the origin
                glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
        );*/
        static float f = 0.f;
        static float x = 0.f;
        static float y = 0.f;
        static float z = 0.f;

        glm::mat4 view = glm::mat4(1);
        {
            view = glm::rotate(view,
                     -glm::radians(0.f) // pitch
                    , {1, 0, 0}
            );
            view = glm::rotate(view,
                     -glm::radians(0.f) //yaw
                    , {0, 1, 0}
            );
            view = glm::rotate(view,
                     0.f // roll
                    , {0, 0, 1}
            );
        }

        view = glm::translate(view, glm::vec3(x, y, z));

        float speed = 0.2;

        if ((GetAsyncKeyState(VK_LEFT) < 0))
            x -= speed;
        if ((GetAsyncKeyState(VK_RIGHT) < 0))
            x += speed;

        if ((GetAsyncKeyState(VK_UP) < 0))
            z += speed;
        if ((GetAsyncKeyState(VK_DOWN) < 0))
            z -= speed;

        if ((GetAsyncKeyState(VK_SPACE) < 0))
            y -= speed;
        if ((GetAsyncKeyState(VK_SHIFT) < 0))
            y += speed;

        int i = 0;
        for (auto & _mesh : meshes) {
            glm::mat4 model = glm::mat4(1);
            model = glm::translate(model, glm::vec3(i * 2.5, 0, -5 * i));
           // model *= glm::mat4(glm::angleAxis(glm::radians(f), glm::vec3(0, 1, 0)));

            i++;

            _mesh.m_ubo = {
                    projectionMatrix,
                    view,
                    model
            };

            _mesh.m_uniformBuffer->CopyToDevice(&_mesh.m_ubo, sizeof(UniformBuffer));
        }
    }

    bool LoadTexture() {
        int w, h, channels;
        uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\Miku\miku.jpeg)", &w, &h, &channels, STBI_rgb_alpha);
        //uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\DDLC\Monika\An exception has occured.jpg)", &w, &h, &channels, STBI_rgb_alpha);
        //uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\akira-(been0328)-Anime-kaguya-sama-wa-kokurasetai-_tensai-tachi-no-renai-zunousen_-Shinomiya-Kaguya-5003254.jpeg)", &w, &h, &channels, STBI_rgb_alpha);
        //uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\Miku\5UyhDcR0p8g.jpg)", &w, &h, &channels, STBI_rgb_alpha);
        if (!pixels) {
            VK_ERROR("Example::LoadTexture() : failed to load texture! Reason: " + std::string(stbi_failure_reason()));
            return false;
        }

        m_texture = Types::Texture::LoadAutoMip(m_device, m_cmdPool, pixels, VK_FORMAT_R8G8B8A8_SRGB, w, h, channels);
        if (!m_texture)
            return false;

        stbi_image_free(pixels);

        return true;
    }

    bool SetupUniforms() {
        for (auto & _mesh : meshes) {
            _mesh.m_descriptorSet = this->m_descriptorManager->AllocateDescriptorSets(
                    m_shader->GetDescriptorSetLayout(),
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}
            );
            if (!_mesh.m_descriptorSet) {
                VK_ERROR("VulkanExample::SetupDescriptors() : failed to allocate descriptor sets!");
                return false;
            }

            //!=============================================================================================================

            _mesh.m_uniformBuffer = EvoVulkan::Types::Buffer::Create(
                    m_device,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                    sizeof(UniformBuffer));

            // Setup a descriptor image info for the current texture to be used as a combined image sampler
            VkDescriptorImageInfo textureDescriptor;
            textureDescriptor.imageView   = m_texture->m_view;			// The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
            textureDescriptor.sampler     = m_texture->m_sampler;		// The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
            textureDescriptor.imageLayout = m_texture->m_imageLayout;	// The current layout of the image (Note: Should always fit the actual use, e.g. shader read)

            std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                    // Binding 0 : Vertex shader uniform buffer
                    Tools::Initializers::WriteDescriptorSet(_mesh.m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                                            &_mesh.m_uniformBuffer->m_descriptor),

                    Tools::Initializers::WriteDescriptorSet(_mesh.m_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                                            &textureDescriptor)
            };
            vkUpdateDescriptorSets(*m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
        }

        return true;
    }
    bool SetupShader() {
        this->m_shader = new Complexes::Shader(GetDevice(), GetRenderPass(), GetPipelineCache());

        m_shader->Load("J://C++/EvoVulkan/Resources/Shaders", "J://C++/EvoVulkan/Resources/Cache",
                       {
                               {"shader.vert", VK_SHADER_STAGE_VERTEX_BIT},
                               {"shader.frag", VK_SHADER_STAGE_FRAGMENT_BIT},
                       },
                       {
                               Tools::Initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                               VK_SHADER_STAGE_VERTEX_BIT, 0),

                               Tools::Initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                                               VK_SHADER_STAGE_FRAGMENT_BIT, 1),
                       },
                       {
                               sizeof(UniformBuffer)
                       });

        m_shader->SetVertexDescriptions(
                { Tools::Initializers::VertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX) },
                {
                    Tools::Initializers::VertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)),
                    Tools::Initializers::VertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv))
                }
        );

        m_shader->Compile(
                VK_POLYGON_MODE_FILL,
                VK_CULL_MODE_NONE,
                VK_COMPARE_OP_LESS_OR_EQUAL,
                VK_FALSE,
                VK_TRUE
        );

        return true;
    }

    bool GenerateGeometry() {
        // Setup vertices for a single uv-mapped quad made from two triangles
        std::vector<Vertex> vertices =
                {
                        { {1.0f,  2.0f,  0.0f}, { 1.0f, 1.0f } },
                        { {-1.0f, 2.0f,  0.0f}, { 0.0f, 1.0f } },
                        { {-1.0f, -1.0f, 0.0f}, { 0.0f, 0.0f } },
                        { {1.0f,  -1.0f, 0.0f}, { 1.0f, 0.0f } }
                };

        // Setup indices
        std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

        // Create buffers
        // For the sake of simplicity we won't stage the vertex data to the gpu memory
        // Vertex buffer
        auto vertexBuffer = Types::Buffer::Create(
                m_device,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vertices.size() * sizeof(Vertex),
                vertices.data());
        // return false;

        // Index buffer
        auto indexBuffer = Types::Buffer::Create(
                m_device,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                indices.size() * sizeof(uint32_t),
                indices.data());

        for (auto &meshe : meshes) {
            meshe.m_indexBuffer  = indexBuffer;
            meshe.m_vertexBuffer = vertexBuffer;
            meshe.m_countIndices = indices.size();
        }

        //auto* mesh = new Complexes::Mesh(m_device, vertexBuffer, indexBuffer, 6, m_descriptorManager);
        //mesh->Bake(m_shader);

        return true;
    }

    bool BuildCmdBuffers() override {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufInfo.pNext = nullptr;

        // Set clear values for all framebuffer attachments with loadOp set to clear
        // We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = m_renderPass;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = m_width;
        renderPassBeginInfo.renderArea.extent.height = m_height;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = m_clearValues;

        for (int i = 0; i < 3; i++) {
            renderPassBeginInfo.framebuffer = m_frameBuffers[i];

            vkBeginCommandBuffer(m_drawCmdBuffs[i], &cmdBufInfo);
            vkCmdBeginRenderPass(m_drawCmdBuffs[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport = Tools::Initializers::Viewport((float)m_width, (float)m_height, 0.0f, 1.0f);
            vkCmdSetViewport(m_drawCmdBuffs[i], 0, 1, &viewport);

            VkRect2D scissor = Tools::Initializers::Rect2D(m_width, m_height, 0, 0);
            vkCmdSetScissor(m_drawCmdBuffs[i], 0, 1, &scissor);

            {
                vkCmdBindPipeline(m_drawCmdBuffs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_shader->GetPipeline());

                //vkCmdDraw(m_drawCmdBuffs[i], 3, 1, 0, 0);

                for (auto & _mesh : meshes) {
                    /*VkDeviceSize offsets[1] = {0};
                    vkCmdBindDescriptorSets(m_drawCmdBuffs[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            m_shader->GetPipelineLayout(), 0, 1, &_mesh.m_descriptorSet, 0, NULL);
                    vkCmdBindVertexBuffers(m_drawCmdBuffs[i], 0, 1, &_mesh.m_vertexBuffer->m_buffer, offsets);
                    vkCmdBindIndexBuffer(m_drawCmdBuffs[i], _mesh.m_indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);

                    vkCmdDrawIndexed(m_drawCmdBuffs[i], 6, 1, 0, 0, 0);*/

                    _mesh.Draw(m_drawCmdBuffs[i], m_shader->GetPipelineLayout());
                }
            }

            vkCmdEndRenderPass(m_drawCmdBuffs[i]);
            vkEndCommandBuffer(m_drawCmdBuffs[i]);
        }

        return true;
    }

    bool Destroy() override {
        if (m_shader) {
            m_shader->Destroy();
            m_shader->Free();
        }

        for (auto & _mesh : meshes)
            _mesh.m_uniformBuffer->Destroy();

        return VulkanKernel::Destroy();
    }

    bool OnResize() override {
        return true;
    }
};

#endif //EVOVULKAN_EXAMPLE_H
