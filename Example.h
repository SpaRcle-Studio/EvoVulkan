//
// Created by Nikita on 05.05.2021.
//

#ifndef EVOVULKAN_EXAMPLE_H
#define EVOVULKAN_EXAMPLE_H

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <GLFW/glfw3.h>

#include <EvoVulkan/Types/Texture.h>

#include <stbi.h>

#include <EvoVulkan/VulkanKernel.h>

#include <EvoVulkan/Types/RenderPass.h>

#include <EvoVulkan/Complexes/Shader.h>
#include <EvoVulkan/Complexes/Mesh.h>
#include <EvoVulkan/Complexes/Framebuffer.h>

using namespace EvoVulkan;

struct ModelUniformBuffer {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
};

struct SkyboxUniformBuffer {
    glm::mat4 proj;
    glm::mat4 view;
    glm::vec3 camPos;
};

struct PPUniformBuffer {
    float gamma;
};

struct VertexUV {
    float position[3];
    float uv[2];
};

struct Vertex {
    float position[3];
};

struct mesh {
    Core::DescriptorManager* m_descrManager  = nullptr;
    Core::DescriptorSet      m_descriptorSet = {};

    Types::Buffer*           m_uniformBuffer = nullptr;
    Types::Buffer*           m_vertexBuffer  = nullptr;
    Types::Buffer*           m_indexBuffer   = nullptr;
    ModelUniformBuffer       m_ubo           = {};
    uint64_t                 m_countIndices  = 0;

    __forceinline void Draw(const VkCommandBuffer& cmd, const VkPipelineLayout& layout) const {
        VkDeviceSize offsets[1] = {0};

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &m_descriptorSet.m_self, 0, NULL);
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer->m_buffer, offsets);
        vkCmdBindIndexBuffer(cmd, m_indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd, m_countIndices, 1, 0, 0, 0);
    }

    void Destroy() {
        EVSafeFreeObject(m_uniformBuffer);

        m_vertexBuffer = nullptr;
        m_indexBuffer  = nullptr;

        if (m_descriptorSet.m_self != VK_NULL_HANDLE) {
            m_descrManager->FreeDescriptorSet(m_descriptorSet);
            m_descriptorSet = { VK_NULL_HANDLE, VK_NULL_HANDLE };
        }

        m_descrManager = nullptr;
        m_countIndices = 0;
    }
};

class VulkanExample : public Core::VulkanKernel {
private:
    Complexes::Shader*          m_geometry            = nullptr;
    Complexes::Shader*          m_skyboxShader        = nullptr;

    Complexes::Shader*          m_postProcessing      = nullptr;
    Core::DescriptorSet         m_PPDescriptorSet     = { };
    Types::Buffer*              m_PPUniformBuffer     = nullptr;

    Types::Buffer*              m_planeVerticesBuff   = nullptr;
    Types::Buffer*              m_planeIndicesBuff    = nullptr;

    Types::Buffer*              m_skyboxVerticesBuff  = nullptr;
    Types::Buffer*              m_skyboxIndicesBuff   = nullptr;

    Types::Texture*             m_texture             = nullptr;

    Complexes::FrameBuffer*     m_offscreen           = nullptr;

    mesh meshes[3];
    mesh skybox;
public:
    void Render() override {
        if (this->PrepareFrame() == Core::FrameResult::OutOfDate)
            this->m_hasErrors = !this->ResizeWindow();

        /*// Command buffer to be submitted to the queue
        m_submitInfo.commandBufferCount = 1;
        m_submitInfo.pCommandBuffers    = &m_drawCmdBuffs[m_currentBuffer];

        // Submit to queue
        auto result = vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &m_submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            VK_ERROR("renderFunction() : failed to queue submit!");
            return;
        }*/


        m_submitInfo.commandBufferCount = 1;

        m_submitInfo.pWaitSemaphores    = &m_syncs.m_presentComplete;
        m_submitInfo.pSignalSemaphores  = &m_offscreen->m_semaphore;
        m_submitInfo.pCommandBuffers    = &m_offscreen->m_cmdBuff;
        auto result = vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &m_submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            VK_ERROR("renderFunction() : failed to first queue submit!");
            return;
        }

        m_submitInfo.pWaitSemaphores    = &m_offscreen->m_semaphore;
        m_submitInfo.pSignalSemaphores  = &m_syncs.m_renderComplete;
        m_submitInfo.pCommandBuffers    = &m_drawCmdBuffs[m_currentBuffer];
        result = vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &m_submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            VK_ERROR("renderFunction() : failed to second queue submit!");
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
            model = glm::translate(model, glm::vec3(i * 2.5, 0, 5 * i));
           // model *= glm::mat4(glm::angleAxis(glm::radians(f), glm::vec3(0, 1, 0)));

            model *= glm::mat4(glm::angleAxis(glm::radians(10.f * (float)i), glm::vec3(0, 1, 0)));

            i++;

            _mesh.m_ubo = {
                    projectionMatrix,
                    glm::translate(view, glm::vec3(x, y, z)),
                    model
            };

            _mesh.m_uniformBuffer->CopyToDevice(&_mesh.m_ubo, sizeof(ModelUniformBuffer));
        }

        SkyboxUniformBuffer ubo = {
                projectionMatrix,
                view,
                glm::vec3(x, y, z)
        };

        skybox.m_uniformBuffer->CopyToDevice(&ubo, sizeof(SkyboxUniformBuffer));
    }

    void LoadSkybox() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;

        if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, R"(J:\C++\GameEngine\Resources\Models\skybox3.obj)"))
            for (const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                    Vertex vertex{};

                    vertex.position[0] = attrib.vertices[3 * index.vertex_index + 0];
                    vertex.position[1] = attrib.vertices[3 * index.vertex_index + 1];
                    vertex.position[2] = attrib.vertices[3 * index.vertex_index + 2];

                    vertices.push_back(vertex);
                    indices.push_back(indices.size());
                }
            }

        //std::cout << indices.size() << std::endl;

        this->m_skyboxVerticesBuff = Types::Buffer::Create(
                m_device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vertices.size() * sizeof(VertexUV),
                vertices.data());

        this->m_skyboxIndicesBuff = Types::Buffer::Create(
                m_device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                indices.size() * sizeof(uint32_t),
                indices.data());

        skybox.m_descriptorSet = this->m_descriptorManager->AllocateDescriptorSets(m_skyboxShader->GetDescriptorSetLayout(), {
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        });
        skybox.m_countIndices = indices.size();
        skybox.m_vertexBuffer = m_skyboxVerticesBuff;
        skybox.m_indexBuffer  = m_skyboxIndicesBuff;
        skybox.m_descrManager = m_descriptorManager;

        skybox.m_uniformBuffer = EvoVulkan::Types::Buffer::Create(
                m_device,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                sizeof(SkyboxUniformBuffer));

        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                // Binding 0 : Fragment shader uniform buffer
                Tools::Initializers::WriteDescriptorSet(skybox.m_descriptorSet.m_self, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                                        &skybox.m_uniformBuffer->m_descriptor),
        };

        vkUpdateDescriptorSets(*m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
    }

    bool LoadTexture() {
        int w, h, channels;
        //uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\Miku\miku.jpeg)", &w, &h, &channels, STBI_rgb_alpha);
        uint8_t* pixels = stbi_load(R"(J:\Photo\Arts\DDLC\Monika\An exception has occured.jpg)", &w, &h, &channels, STBI_rgb_alpha);
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

    bool UpdatePP() {
        auto attach0 = m_offscreen->GetImageDescriptors()[0];
        auto attach1 = m_offscreen->GetImageDescriptors()[1];
        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                // Binding 0 : Fragment shader uniform buffer
                Tools::Initializers::WriteDescriptorSet(m_PPDescriptorSet.m_self, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                                        &m_PPUniformBuffer->m_descriptor),

                // Binding 1: Fragment shader sampler
                Tools::Initializers::WriteDescriptorSet(m_PPDescriptorSet.m_self, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                                        &attach0),

                // Binding 2: Fragment shader sampler
                Tools::Initializers::WriteDescriptorSet(m_PPDescriptorSet.m_self, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
                                                        &attach1)
        };

        vkUpdateDescriptorSets(*m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

        return true;
    }

    bool SetupUniforms() {
        // geometry
        for (auto & _mesh : meshes) {
            _mesh.m_descrManager = m_descriptorManager;

            _mesh.m_descriptorSet = this->m_descriptorManager->AllocateDescriptorSets(
                    m_geometry->GetDescriptorSetLayout(),
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}
            );
            if (_mesh.m_descriptorSet.m_self == VK_NULL_HANDLE) {
                VK_ERROR("VulkanExample::SetupDescriptors() : failed to allocate descriptor sets!");
                return false;
            }

            //!=============================================================================================================

            _mesh.m_uniformBuffer = EvoVulkan::Types::Buffer::Create(
                    m_device,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                    sizeof(ModelUniformBuffer));

            // Setup a descriptor image info for the current texture to be used as a combined image sampler
            VkDescriptorImageInfo textureDescriptor;
            textureDescriptor.imageView   = m_texture->m_view;			// The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
            textureDescriptor.sampler     = m_texture->m_sampler;		// The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
            textureDescriptor.imageLayout = m_texture->m_imageLayout;	// The current layout of the image (Note: Should always fit the actual use, e.g. shader read)

            std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                    // Binding 0 : Vertex shader uniform buffer
                    Tools::Initializers::WriteDescriptorSet(_mesh.m_descriptorSet.m_self, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                                            &_mesh.m_uniformBuffer->m_descriptor),

                    Tools::Initializers::WriteDescriptorSet(_mesh.m_descriptorSet.m_self, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                                            &textureDescriptor)
            };
            vkUpdateDescriptorSets(*m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
        }

        // post processing
        this->m_PPDescriptorSet = this->m_descriptorManager->AllocateDescriptorSets(m_postProcessing->GetDescriptorSetLayout(), {
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        });

        m_PPUniformBuffer = EvoVulkan::Types::Buffer::Create(
                m_device,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                sizeof(PPUniformBuffer));

        return UpdatePP();
    }
    bool SetupShader() {
        //this->m_geometry = new Complexes::Shader(GetDevice(), GetRenderPass(), GetPipelineCache());
        this->m_geometry = new Complexes::Shader(GetDevice(), m_offscreen->GetRenderPass(), GetPipelineCache());

        m_geometry->Load("J://C++/EvoVulkan/Resources/Shaders", "J://C++/EvoVulkan/Resources/Cache",
                         {
                                 {"geometry.vert", VK_SHADER_STAGE_VERTEX_BIT},
                                 {"geometry.frag", VK_SHADER_STAGE_FRAGMENT_BIT},
                         },
                         {
                                 Tools::Initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                                 VK_SHADER_STAGE_VERTEX_BIT, 0),

                                 Tools::Initializers::DescriptorSetLayoutBinding(
                                         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                         VK_SHADER_STAGE_FRAGMENT_BIT, 1),

                                 Tools::Initializers::DescriptorSetLayoutBinding(
                                         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                         VK_SHADER_STAGE_FRAGMENT_BIT, 2),
                         },
                         {
                                 sizeof(ModelUniformBuffer)
                         });

        m_geometry->SetVertexDescriptions(
                {Tools::Initializers::VertexInputBindingDescription(0, sizeof(VertexUV), VK_VERTEX_INPUT_RATE_VERTEX)},
                {
                        Tools::Initializers::VertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                                                                             offsetof(VertexUV, position)),
                        Tools::Initializers::VertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT,
                                                                             offsetof(VertexUV, uv))
                }
        );

        m_geometry->Compile(
                VK_POLYGON_MODE_FILL,
                VK_CULL_MODE_NONE,
                VK_COMPARE_OP_LESS_OR_EQUAL,
                VK_TRUE,
                VK_TRUE
        );

        //!=============================================================================================================

        this->m_skyboxShader = new Complexes::Shader(GetDevice(), m_offscreen->GetRenderPass(), GetPipelineCache());

        m_skyboxShader->Load("J://C++/EvoVulkan/Resources/Shaders", "J://C++/EvoVulkan/Resources/Cache",
                         {
                                 {"skybox.vert", VK_SHADER_STAGE_VERTEX_BIT},
                                 {"skybox.frag", VK_SHADER_STAGE_FRAGMENT_BIT},
                         },
                         {
                                 Tools::Initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                                 VK_SHADER_STAGE_VERTEX_BIT, 0),

                                 //Tools::Initializers::DescriptorSetLayoutBinding(
                                 //        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                 //        VK_SHADER_STAGE_FRAGMENT_BIT, 1),
                         },
                         {
                                 sizeof(SkyboxUniformBuffer)
                         });

        m_skyboxShader->SetVertexDescriptions(
                {Tools::Initializers::VertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)},
                {
                        Tools::Initializers::VertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                                                                             offsetof(Vertex, position)),
                }
        );

        m_skyboxShader->Compile(
                VK_POLYGON_MODE_FILL,
                VK_CULL_MODE_NONE,
                VK_COMPARE_OP_LESS_OR_EQUAL,
                VK_TRUE,
                VK_TRUE
        );

        //!=============================================================================================================

        //this->m_postProcessing = new Complexes::Shader(GetDevice(), this->m_offscreen->GetRenderPass(), GetPipelineCache());
        this->m_postProcessing = new Complexes::Shader(GetDevice(), this->GetRenderPass(), GetPipelineCache());

        m_postProcessing->Load("J://C++/EvoVulkan/Resources/Shaders", "J://C++/EvoVulkan/Resources/Cache",
                               {
                                       {"post_processing.vert", VK_SHADER_STAGE_VERTEX_BIT},
                                       {"post_processing.frag", VK_SHADER_STAGE_FRAGMENT_BIT},
                               },
                               {
                                       Tools::Initializers::DescriptorSetLayoutBinding(
                                               VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                               VK_SHADER_STAGE_FRAGMENT_BIT, 0),

                                       Tools::Initializers::DescriptorSetLayoutBinding(
                                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                               VK_SHADER_STAGE_FRAGMENT_BIT, 1),

                                       Tools::Initializers::DescriptorSetLayoutBinding(
                                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                               VK_SHADER_STAGE_FRAGMENT_BIT, 2),
                               },
                               {
                                       sizeof(PPUniformBuffer)
                               });

        m_postProcessing->Compile(
                VK_POLYGON_MODE_FILL,
                VK_CULL_MODE_NONE,
                VK_COMPARE_OP_LESS_OR_EQUAL,
                VK_TRUE,
                VK_TRUE
        );

        return true;
    }

    bool GenerateGeometry() {
        // Setup vertices for a single uv-mapped quad made from two triangles
        std::vector<VertexUV> vertices =
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
        this->m_planeVerticesBuff = Types::Buffer::Create(
                m_device,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vertices.size() * sizeof(VertexUV),
                vertices.data());
        // return false;

        // Index buffer
        this->m_planeIndicesBuff = Types::Buffer::Create(
                m_device,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                indices.size() * sizeof(uint32_t),
                indices.data());

        for (auto &meshe : meshes) {
            meshe.m_indexBuffer  = m_planeIndicesBuff;
            meshe.m_vertexBuffer = m_planeVerticesBuff;
            meshe.m_countIndices = indices.size();
        }

        //auto* mesh = new Complexes::Mesh(m_device, vertexBuffer, indexBuffer, 6, m_descriptorManager);
        //mesh->Bake(m_shader);

        return true;
    }

    bool BuildCmdBuffers123123123() {
        VkCommandBufferBeginInfo cmdBufInfo = Tools::Initializers::CommandBufferBeginInfo();

        VkClearValue clearValues[2] {
                { .color = {{0.5f, 0.5f, 0.5f, 1.0f}} },
                { .depthStencil = { 1.0f, 0 } }
        };

        auto renderPassBI = Tools::Insert::RenderPassBeginInfo(
                m_width, m_height, m_renderPass.m_self,
                VK_NULL_HANDLE, &clearValues[0], 2);

        for (int i = 0; i < 3; i++) {
            renderPassBI.framebuffer = m_frameBuffers[i];

            vkBeginCommandBuffer(m_drawCmdBuffs[i], &cmdBufInfo);
            vkCmdBeginRenderPass(m_drawCmdBuffs[i], &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport = Tools::Initializers::Viewport((float) m_width, (float) m_height, 0.0f, 1.0f);
            vkCmdSetViewport(m_drawCmdBuffs[i], 0, 1, &viewport);

            VkRect2D scissor = Tools::Initializers::Rect2D(m_width, m_height, 0, 0);
            vkCmdSetScissor(m_drawCmdBuffs[i], 0, 1, &scissor);

            vkCmdBindPipeline(m_drawCmdBuffs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_geometry);

            for (auto &_mesh : meshes)
                _mesh.Draw(m_drawCmdBuffs[i], m_geometry->GetPipelineLayout());

            vkCmdEndRenderPass(m_drawCmdBuffs[i]);
            vkEndCommandBuffer(m_drawCmdBuffs[i]);
        }

        return true;
    }

    bool BuildCmdBuffers() override {
        const uint8_t countClsVals = 3;

        VkClearValue clearValues[countClsVals] {
                { .color = {{0.0f, 0.0f, 0.0f, 1.0f}} },
                { .color = {{0.0f, 0.0f, 0.0f, 1.0f}} },
                { .depthStencil = { 1.0f, 0 } }
        };

        VkRenderPassBeginInfo renderPassBeginInfo = m_offscreen->BeginRenderPass(&clearValues[0], countClsVals);

        m_offscreen->BeginCmd();
        vkCmdBeginRenderPass(m_offscreen->GetCmd(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        m_offscreen->SetViewportAndScissor();

        {
            vkCmdBindPipeline(m_offscreen->GetCmd(), VK_PIPELINE_BIND_POINT_GRAPHICS, *m_geometry);

            for (auto & _mesh : meshes)
                _mesh.Draw(m_offscreen->GetCmd(), m_geometry->GetPipelineLayout());
        }

        {
            vkCmdBindPipeline(m_offscreen->m_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_skyboxShader);

            skybox.Draw(m_offscreen->m_cmdBuff, m_skyboxShader->GetPipelineLayout());
        }

        m_offscreen->End();

        return BuildCmdBuffersPostProcess();
    }

    bool BuildCmdBuffersPostProcess() {
        VkCommandBufferBeginInfo cmdBufInfo = Tools::Initializers::CommandBufferBeginInfo();

        VkClearValue clearValues[2] {
                { .color = {{0.5f, 0.5f, 0.5f, 1.0f}} },
                { .depthStencil = { 1.0f, 0 } }
        };

        auto renderPassBI = Tools::Insert::RenderPassBeginInfo(
                m_width, m_height, m_renderPass.m_self,
                VK_NULL_HANDLE, &clearValues[0], 2);

        for (int i = 0; i < 3; i++) {
            renderPassBI.framebuffer = m_frameBuffers[i];

            vkBeginCommandBuffer(m_drawCmdBuffs[i], &cmdBufInfo);
            vkCmdBeginRenderPass(m_drawCmdBuffs[i], &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport = Tools::Initializers::Viewport((float)m_width, (float)m_height, 0.0f, 1.0f);
            vkCmdSetViewport(m_drawCmdBuffs[i], 0, 1, &viewport);

            VkRect2D scissor = Tools::Initializers::Rect2D(m_width, m_height, 0, 0);
            vkCmdSetScissor(m_drawCmdBuffs[i], 0, 1, &scissor);

            vkCmdBindPipeline(m_drawCmdBuffs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *m_postProcessing);
            vkCmdBindDescriptorSets(m_drawCmdBuffs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_postProcessing->GetPipelineLayout(), 0, 1, &m_PPDescriptorSet.m_self, 0, NULL);

            vkCmdDraw(m_drawCmdBuffs[i], 3, 1, 0, 0);

            vkCmdEndRenderPass(m_drawCmdBuffs[i]);
            vkEndCommandBuffer(m_drawCmdBuffs[i]);
        }

        return true;
    }

    bool Destroy() override {
        VK_LOG("Example::Destroy() : destroy kernel inherit class...");

        EVSafeFreeObject(m_offscreen);

        EVSafeFreeObject(m_texture);

        EVSafeFreeObject(m_PPUniformBuffer);
        if (m_PPDescriptorSet.m_self != VK_NULL_HANDLE) {
            this->m_descriptorManager->FreeDescriptorSet(m_PPDescriptorSet);
            m_PPDescriptorSet = { VK_NULL_HANDLE, VK_NULL_HANDLE };
        }

        EVSafeFreeObject(m_geometry);
        EVSafeFreeObject(m_postProcessing);
        EVSafeFreeObject(m_skyboxShader);

        for (auto & _mesh : meshes)
            _mesh.Destroy();
        skybox.Destroy();

        EVSafeFreeObject(m_skyboxVerticesBuff);
        EVSafeFreeObject(m_skyboxIndicesBuff);

        EVSafeFreeObject(m_planeVerticesBuff);
        EVSafeFreeObject(m_planeIndicesBuff);

        return VulkanKernel::Destroy();
    }

    bool OnComplete() override {
        this->m_offscreen = Complexes::FrameBuffer::Create(
                m_device,
                m_swapchain,
                m_cmdPool,
                {
                        VK_FORMAT_R32G32B32A32_SFLOAT,
                        VK_FORMAT_R32G32B32A32_SFLOAT,
                        //VK_FORMAT_R8G8B8A8_UNORM,
                },
                m_width,
                m_height,
                1);

        if (!m_offscreen)
            return false;

        return true;
    }

    bool OnResize() override {
        vkQueueWaitIdle(m_device->GetGraphicsQueue());
        vkDeviceWaitIdle(*m_device);

        return m_offscreen ? (this->m_offscreen->ReCreate(m_width, m_height) && UpdatePP()) : true;
        //return true;
    }
};

#endif //EVOVULKAN_EXAMPLE_H
