/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2018-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vulkan/vulkan.h>

/**

\fn load_VK_EXTENSIONS
\brief load_VK_EXTENSIONS : Vulkan Extension Loader

The extensions_vk files takes care of loading and providing the symbols of Vulkan C Api extensions.
It is generated by `extensions_vk.py` and generates all extensions found in vk.xml. See script for details.
.

The framework triggers this implicitly in the `nvvk::Context` class, immediately after creating the device.

\code{.cpp}
// loads all known extensions
load_VK_EXTENSIONS(instance, vkGetInstanceProcAddr, device, vkGetDeviceProcAddr);
\endcode

*/

/* Load all available extensions */
void load_VK_EXTENSIONS(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr);

