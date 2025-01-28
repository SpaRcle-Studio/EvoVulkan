//
// Created by Monika on 19.07.2023.
//

#ifndef SR_ENGINE_SUBMITINFO_H
#define SR_ENGINE_SUBMITINFO_H

#include <EvoVulkan/Memory/Allocator.h>
#include <EvoVulkan/Tools/VulkanInitializers.h>

namespace EvoVulkan {
    struct SubmitInfo {
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkSemaphore> signalSemaphores;

        void AddWaitSemaphore(VkSemaphore semaphore) {
            if (std::find(waitSemaphores.begin(), waitSemaphores.end(), semaphore) != waitSemaphores.end()) {
                return;
            }

            waitSemaphores.emplace_back(semaphore);
        }

        void AddSignalSemaphore(VkSemaphore semaphore) {
            if (std::find(signalSemaphores.begin(), signalSemaphores.end(), semaphore) != signalSemaphores.end()) {
                return;
            }

            signalSemaphores.emplace_back(semaphore);
        }

        void SetWaitDstStageMask(VkPipelineStageFlags flags) {
            waitDstStageMask = flags;
            waitDstStageMasks.clear();
        }

        EVK_NODISCARD VkSubmitInfo ToVk() const noexcept {
            auto&& submitInfo = Tools::Initializers::SubmitInfo();

            if (waitSemaphores.size() != waitDstStageMasks.size()) {
                waitDstStageMasks.resize(waitSemaphores.size());
                for (auto&& flag : waitDstStageMasks) {
                    flag = waitDstStageMask;
                }
            }

            submitInfo.pWaitDstStageMask = waitDstStageMasks.data();

            submitInfo.waitSemaphoreCount = waitSemaphores.size();
            submitInfo.pWaitSemaphores = waitSemaphores.data();

            submitInfo.signalSemaphoreCount = signalSemaphores.size();
            submitInfo.pSignalSemaphores = signalSemaphores.data();

            submitInfo.commandBufferCount = commandBuffers.size();
            submitInfo.pCommandBuffers = commandBuffers.data();

            return submitInfo;
        }

    private:
        VkPipelineStageFlags waitDstStageMask = 0;
        mutable std::vector<VkPipelineStageFlags> waitDstStageMasks;

    };
}

#endif //SR_ENGINE_SUBMITINFO_H
