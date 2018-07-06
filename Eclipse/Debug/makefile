################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Utility/subdir.mk
-include Server/Utility/subdir.mk
-include Server/Scenes/NetworkScene/subdir.mk
-include Server/subdir.mk
-include Scenes/WarScene/AESOPActions/subdir.mk
-include Scenes/WarScene/subdir.mk
-include Scenes/TenisScene/subdir.mk
-include Scenes/PingPongScene/subdir.mk
-include Scenes/PhysXScene/subdir.mk
-include Scenes/NetworkScene/Network/subdir.mk
-include Scenes/NetworkScene/subdir.mk
-include Scenes/MainScene/subdir.mk
-include Scenes/FlashScene/subdir.mk
-include Scenes/CubeScene/subdir.mk
-include Scenes/subdir.mk
-include Rendering/RenderPass/subdir.mk
-include Rendering/PostFX/CustomOperators/subdir.mk
-include Rendering/PostFX/subdir.mk
-include Rendering/Lighting/ShadowMapping/subdir.mk
-include Rendering/Lighting/subdir.mk
-include Rendering/Camera/subdir.mk
-include Rendering/subdir.mk
-include Platform/Video/Textures/subdir.mk
-include Platform/Video/Shaders/subdir.mk
-include Platform/Video/OpenGL/glsw/subdir.mk
-include Platform/Video/OpenGL/Textures/subdir.mk
-include Platform/Video/OpenGL/Shaders/subdir.mk
-include Platform/Video/OpenGL/Buffers/VertexBuffer/subdir.mk
-include Platform/Video/OpenGL/Buffers/ShaderBuffer/subdir.mk
-include Platform/Video/OpenGL/Buffers/PixelBuffer/subdir.mk
-include Platform/Video/OpenGL/Buffers/Framebuffer/subdir.mk
-include Platform/Video/OpenGL/Buffers/subdir.mk
-include Platform/Video/OpenGL/subdir.mk
-include Platform/Video/Direct3D/Textures/subdir.mk
-include Platform/Video/Direct3D/Shaders/subdir.mk
-include Platform/Video/Direct3D/Buffers/VertexBuffer/subdir.mk
-include Platform/Video/Direct3D/Buffers/ShaderBuffer/subdir.mk
-include Platform/Video/Direct3D/Buffers/PixelBuffer/subdir.mk
-include Platform/Video/Direct3D/Buffers/Framebuffer/subdir.mk
-include Platform/Video/Direct3D/subdir.mk
-include Platform/Video/Buffers/ShaderBuffer/subdir.mk
-include Platform/Video/Buffers/Framebuffer/subdir.mk
-include Platform/Video/subdir.mk
-include Platform/Threading/subdir.mk
-include Platform/Platform/subdir.mk
-include Platform/Input/subdir.mk
-include Platform/Audio/sdl_mixer/subdir.mk
-include Platform/Audio/openAl/subdir.mk
-include Platform/Audio/subdir.mk
-include Physics/PhysX/Actors/subdir.mk
-include Physics/PhysX/subdir.mk
-include Physics/subdir.mk
-include Networking/subdir.mk
-include Managers/subdir.mk
-include Libs/ReCast/RecastContrib/fastlz/subdir.mk
-include Libs/ReCast/ReCast/Source/subdir.mk
-include Libs/ReCast/DetourTileCache/Source/subdir.mk
-include Libs/ReCast/DetourCrowd/Source/subdir.mk
-include Libs/ReCast/Detour/Source/subdir.mk
-include Libs/ReCast/DebugUtils/Source/subdir.mk
-include Libs/GLIM/subdir.mk
-include Libs/CPPGoap/subdir.mk
-include Graphs/Components/subdir.mk
-include Graphs/subdir.mk
-include Geometry/Shapes/subdir.mk
-include Geometry/Material/subdir.mk
-include Geometry/Importer/subdir.mk
-include Geometry/Animations/subdir.mk
-include GUI/GUIEditor/subdir.mk
-include GUI/CEGUIAddons/subdir.mk
-include GUI/subdir.mk
-include Environment/Water/subdir.mk
-include Environment/Vegetation/subdir.mk
-include Environment/Terrain/Quadtree/subdir.mk
-include Environment/Terrain/subdir.mk
-include Environment/Sky/subdir.mk
-include Dynamics/WeaponSystem/Weapons/subdir.mk
-include Dynamics/WeaponSystem/Projectile/subdir.mk
-include Dynamics/WeaponSystem/Ammunition/subdir.mk
-include Dynamics/Entities/Units/subdir.mk
-include Dynamics/Entities/Triggers/subdir.mk
-include Dynamics/Entities/Particles/ConcreteUpdaters/subdir.mk
-include Dynamics/Entities/Particles/ConcreteGenerators/subdir.mk
-include Dynamics/Entities/Particles/subdir.mk
-include Dynamics/Entities/subdir.mk
-include Core/Resources/ConcreteLoaders/subdir.mk
-include Core/Resources/subdir.mk
-include Core/MemoryManagement/subdir.mk
-include Core/Math/BoundingVolumes/subdir.mk
-include Core/Math/subdir.mk
-include Core/subdir.mk
-include AI/Sensors/subdir.mk
-include AI/PathFinding/Waypoints/subdir.mk
-include AI/PathFinding/NavMeshes/DetourTileCache/subdir.mk
-include AI/PathFinding/NavMeshes/subdir.mk
-include AI/PathFinding/subdir.mk
-include AI/ActionInterface/subdir.mk
-include AI/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: Divide-Game

# Tool invocations
Divide-Game: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "Divide-Game" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C++_DEPS)$(C_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(CXX_DEPS)$(C_UPPER_DEPS) Divide-Game
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
