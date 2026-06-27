//
// Created by Giovanni Bollati on 09/06/25.
//

#include <Rendering/PSOConfigs.hpp>
#include <Rendering/VertexDescriptor.hpp>
#include <string>
#include <Texture.hpp>

namespace Rendering
{
    const std::unordered_map<std::string, const PSOConfig> psoConfigs = {
        std::pair<std::string, const PSOConfig>{
            std::string("Unlit"),
            PSOConfig
            {
                "Unlit",
                PipelineType::Vertex,
                "vertexUnlitShader",
                "fragmentUnlitShader",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PC"),
                {
                    {PREVIOUS_MODEL_MATRIX, Vertex, 25},
                    {MODEL_MATRIX, Vertex, 27},
                    {UNLIT_COLOR, Vertex, 26}
                }, {
                        {VIEW_PROJECTION_MATRIX, Vertex, 28},
                        {PREVIOUS_VIEW_PROJECTION_MATRIX, Vertex, 29},
                        {JITTERED_VIEW_PROJECTION_MATRIX, Vertex, 30},
                }, {}, {}, {}
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("VCPHONG"),
            PSOConfig
            {
                "VCPHONG",
                PipelineType::Vertex,
                "vertexPHONG",
                "fragmentVCPHONG",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PCNUV"),
                {
                    {NORMAL_MATRIX, Vertex, 27},
                    {MODEL_MATRIX, Vertex, 28},
                    {SHININESS, Fragment, 26}
                    },
                {
                    {VIEW_MATRIX, Vertex, 29},
                    {PROJECTION_MATRIX, Vertex, 30},
                    {CAMERA_POSITION, Fragment, 27},
                    {LIGHTS, Fragment, 28}
                },
                    {}, {},
                    {}
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("VCWARD"),
            PSOConfig
            {
                "VCWARD",
                PipelineType::Vertex,
                "vertexWARD",
                "fragmentVCWARD",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PCNUV"),
                {
                    {ROUGHNESS, Fragment, 25},
                    {METALLIC, Fragment, 26},
                    {NORMAL_MATRIX, Vertex, 27},
                    {MODEL_MATRIX, Vertex, 28},
                    },
                {
                    {VIEW_MATRIX, Vertex, 29},
                    {PROJECTION_MATRIX, Vertex, 30},
                    {CAMERA_POSITION, Fragment, 27},
                    {LIGHTS, Fragment, 28}
                },
                    {}, {}, {}
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("VCWARD_SHADOW"),
            PSOConfig
            {
                "VCWARD_SHADOW",
                PipelineType::Vertex,
                "vertexWARDSHADOW",
                "fragmentVCWARDSHADOW",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PCNUV"),
                {
                        {ROUGHNESS, Fragment, 25},
                        {METALLIC, Fragment, 26},
                        {NORMAL_MATRIX, Vertex, 27},
                        {MODEL_MATRIX, Vertex, 28},
                        },
                    {
                        {SHADOW_DATA, Fragment, 24},
                        {VIEW_MATRIX, Vertex, 29},
                        {PROJECTION_MATRIX, Vertex, 30},
                        {CAMERA_POSITION, Fragment, 27},
                        {LIGHTS, Fragment, 28}
                    },
                        {
                        }, {
                        {Core::ShadowMap, Fragment, 0}
                        },
                        {
                                {Core::MirrorRepeat,
                                Core::MirrorRepeat,
                                    Core::None,
                                Core::Linear,
                                Core::Linear,
                                true,
                                Fragment, 0
                                }
                        }
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("VCWARD_FULLSHADOW"),
            PSOConfig
            {
                "VCWARD_FULLSHADOW",
                PipelineType::Vertex,
                "vertexWARDSHADOW",
                "fragmentVCWARDFULLSHADOW",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PCNUV"),
                {
                            {PREVIOUS_MODEL_MATRIX, Vertex, 25},
                            {WARD_ALPHA, Fragment, 22},
                            {ROUGHNESS, Fragment, 25},
                            {METALLIC, Fragment, 26},
                            {NORMAL_MATRIX, Vertex, 26},
                            {MODEL_MATRIX, Vertex, 27},
                            },
                        {
                            {SHADOW_DATA, Fragment, 23},
                            {POINT_SHADOW_DATA, Fragment, 24},
                            {VIEW_PROJECTION_MATRIX, Vertex, 28},
                            {PREVIOUS_VIEW_PROJECTION_MATRIX, Vertex, 29},
                            {JITTERED_VIEW_PROJECTION_MATRIX, Vertex, 30},
                            {CAMERA_POSITION, Fragment, 27},
                            {LIGHTS, Fragment, 28}
                        },
                            {
                            }, {
                                {Core::ShadowMap, Fragment, 0},
                                {Core::CubeShadowMap, Fragment, 1}
                            },
                            {
                                    {Core::MirrorRepeat,
                                    Core::MirrorRepeat,
                                        Core::None,
                                    Core::Nearest,
                                    Core::Nearest,
                                    true,
                                    Fragment, 0
                                    }
                            }
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("VCGOURAUD"),
            PSOConfig
            {
                "VCGOURAUD",
                PipelineType::Vertex,
                "vertexGOURAUD",
                "fragmentVCGOURAUD",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PCNUV"),
                {
                    {NORMAL_MATRIX, Vertex, 27},
                    {MODEL_MATRIX, Vertex, 28},
                    {SHININESS, Vertex, 26}
                    },
                {
                    {VIEW_MATRIX, Vertex, 29},
                    {PROJECTION_MATRIX, Vertex, 30},
                    {CAMERA_POSITION, Vertex, 25},
                    {LIGHTS, Vertex, 24}
                },
                    {}, {},{}
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("UI"),
            PSOConfig
            {
                "UI",
                PipelineType::Vertex,
                "vertexUI",
                "fragmentUI",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PC"),
                {
                    {RECT, Vertex, 2}
                }, {
                {VIEWPORT_SIZE, Vertex, 3}
                }, {}, {}
            },
        },
        std::pair<std::string, const PSOConfig>{
            std::string("TexturePHONG"),
            PSOConfig
            {
                "TexturePHONG",
                PipelineType::Vertex,
                "vertexPHONG",
                "fragmentTexturePHONG",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PCNUV"),
                {
                        {NORMAL_MATRIX, Vertex, 27},
                        {MODEL_MATRIX, Vertex, 28},
                        {SHININESS, Fragment, 26}

                },
                {
                        {VIEW_MATRIX, Vertex, 29},
                        {PROJECTION_MATRIX, Vertex, 30},
                        {CAMERA_POSITION, Fragment, 27},
                        {LIGHTS, Fragment, 28}
                },
                {{Core::Diffuse, Fragment, 0}},
                {},
                {
                        {Core::Repeat,
                        Core::Repeat,
                        Core::None,
                        Core::Nearest,
                        Core::Nearest,
                        true,
                        Fragment, 0}}
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("TextureUI"),
            PSOConfig
            {
                "TextureUI",
                PipelineType::Vertex,
                "vertexTextureUI",
                "fragmentTextureUI",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PUV"),
                {
                    {RECT, Vertex, 2}
                }, {
                    {VIEWPORT_SIZE, Vertex, 3}
                },
                {
                    {Core::Diffuse, Fragment, 0},
                }, {},
                {
                        {Core::Repeat,
                            Core::Repeat,
                            Core::None,
                            Core::Nearest,
                            Core::Nearest,
                            true,
                            Fragment, 0}
                        }
                }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("Depth"),
            PSOConfig
            {
                "Depth",
                PipelineType::Vertex,
                "vertexTextureUI",
                "fragmentDepth",
                "",
                Triangle,
                Solid,
                Back,
                Disabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::DepthInvalid,
                G_getVertexDescriptors().at("PUV"),
                {
                        {RECT, Vertex, 2}
                }, {
                        {VIEWPORT_SIZE, Vertex, 3}
                },
                {
                        {Core::ShadowMap, Fragment, 0}
                }, {}, {
                            {Core::ClampToZero,
                                Core::ClampToZero,
                                Core::None,
                                Core::Linear,
                                Core::Linear,
                                true,
                                Fragment, 0}}
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("curve"),
            PSOConfig
            {
                "curve",
                PipelineType::Vertex,
                "vertexCurve",
                "fragmentCurve",
                "",
                Line,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PC"),

                {
                    {MODEL_MATRIX, Vertex, 26}
                },
                {
                        {VIEW_PROJECTION_MATRIX, Vertex, 27},
                        {PREVIOUS_VIEW_PROJECTION_MATRIX, Vertex, 28},
                        {JITTERED_VIEW_PROJECTION_MATRIX, Vertex, 29},
                }, {}, {}, {}
            }
        },
            std::pair<std::string, const PSOConfig>{
                std::string("DirectionalShadow"),
                PSOConfig
                {
                    "DirectionalShadow",
                    PipelineType::Vertex,
                    "vertexDirectionalShadow",
                    "fragmentDirectionalShadow",
                    "",
                    Triangle,
                    Solid,
                    Back,
                    Enabled,
                    Default,
                    ColorPixelFormat::ColorInvalid,
                    DepthPixelFormat::Depth32Float,
                    G_getVertexDescriptors().at("P"),

                    {
                        {MODEL_MATRIX, Vertex, 28}
                    },
                    {
                            //{DIRECTIONAL_LIGHT_INDEX, Vertex, 29},
                            {SHADOW_DATA, Vertex, 30}
                    }, {}, {}, {}, -1, false
                }
            },
        std::pair<std::string, const PSOConfig>{
            std::string("PointShadow"),
            PSOConfig
            {
                "PointShadow",
                PipelineType::Vertex,
                "vertexPointShadow",
                "fragmentPointShadow",
                "",
                Triangle,
                Solid,
                Front,
                Enabled,
                Default,
                ColorPixelFormat::ColorInvalid,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("P"),

                {
                            {MODEL_MATRIX, Vertex, 28}
                },
                {
                                {POINT_SHADOW_DATA, Vertex, 30},
                                {POINT_SHADOW_DATA, Fragment, 30},

                }, {}, {}, {}, -1, false
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("Skybox"),
            PSOConfig
            {
                "Skybox",
                PipelineType::Vertex,
                "vertexSkybox",
                "fragmentSkybox",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("P"),

                {
                                    {RECT, Vertex, 2}
                },
                {
                                        {VIEWPORT_SIZE, Vertex, 3},
                                        {JITTER, Vertex, 4},
                                        {JITTER, Fragment, 25},
                                        {INVERSE_VIEW_PROJECTION_MATRIX, Vertex, 5},
                                        {VIEW_PROJECTION_MATRIX, Vertex, 6},
                                        {PREVIOUS_VIEW_PROJECTION_MATRIX, Vertex, 7},
                                        {CAMERA_POSITION, Vertex, 8},
                                        {TAAScaling, Vertex, 9},
                                        {PROJECTION_MATRIX, Vertex, 10},
                                        {INVERSE_PROJECTION_MATRIX, Vertex, 11},
                                        {INVERSE_VIEW_MATRIX, Fragment, 26},
                                        {INVERSE_PROJECTION_MATRIX, Fragment, 27},
                                        {CAMERA_POSITION, Fragment, 28},
                                        {VIEWPORT_SIZE, Fragment, 29},
                                        {SUN_DIRECTION, Fragment, 30},
                                        {POTENTIAL_OCTREE, Fragment, 12},
                                        {POTENTIAL_OCTREE_INFO, Fragment, 13},
                                        {BVH_NODES, Fragment, 14},
                                        {BVH_PRIMITIVES, Fragment, 15},
                                        {BVH_INFO, Fragment, 16},
                                        {POTENTIAL_SAMPLING_INFO, Fragment, 17},
                                        {SUN_COLOR, Fragment, 18},
                                        {SHADOW_DATA, Fragment, 19},
                                        {LIGHTS, Fragment, 20},
                                    {ATMOSPHERE_SETTINGS, Fragment, 21}
                }, {}, {
                    {Core::AtmosphereDensity3D, Fragment, 0},
                    {Core::LightTransmittance3D, Fragment, 1},
                    {Core::ShadowMap, Fragment, 2},
                }, {
                    {Core::ClampToEdge, Core::ClampToEdge, Core::ClampToEdge, Core::Linear, Core::Linear, true, Fragment, 0},
                }
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("TestMesh"),
            PSOConfig
            {
                "TestMesh",
                PipelineType::Mesh,
                "",  // no object shader for now
                "testMeshShader",  // mesh shader name (mapped to fragmentShader in PSOConfig for Mesh type)
                "testMeshFragment",  // fragment shader name (mapped to meshFragmentShader in PSOConfig)
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PCN"),
                {
                    {MODEL_MATRIX, Mesh, 28},
                    {NORMAL_MATRIX, Mesh, 27},
                },
                {
                    {VIEW_MATRIX, Mesh, 29},
                    {PROJECTION_MATRIX, Mesh, 30},
                },
                {}, {}, {}, 3
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("Icosphere"),
            PSOConfig
            {
                "Icosphere",
                PipelineType::Mesh,
                "",
                "icosphere_mesh_shader",
                "icosphere_fragment_shader",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("P"), // Dummy
                {
                    {MODEL_MATRIX, Mesh, 28}
                },
                {
                    {VIEW_MATRIX, Mesh, 29},
                    {PROJECTION_MATRIX, Mesh, 30}
                },
                {}, {}, {}
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("DynamicIcosphere"),
            PSOConfig
            {
                "DynamicIcosphere",
                PipelineType::Mesh,
                "dynamic_icosphere_object_shader", // Object shader
                "dynamic_icosphere_mesh_shader",   // Mesh shader
                "dynamic_icosphere_fragment_shader", // Fragment shader
                Triangle,
                Solid,
                None,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("P"), // Dummy
                {
                    {MODEL_MATRIX, Object, 22},
                    {MODEL_MATRIX, Mesh, 22},
                    {PLANET_CP, Object, 10},
                    {PLANET_CP, Mesh, 10},
                    {PLANET_CP, Fragment, 10},
                    {PLANET_KNOTS_U, Object, 11},
                    {PLANET_KNOTS_U, Mesh, 11},
                    {PLANET_KNOTS_U, Fragment, 11},
                    {PLANET_KNOTS_V, Object, 12},
                    {PLANET_KNOTS_V, Mesh, 12},
                    {PLANET_KNOTS_V, Fragment, 12},
                    {PLANET_INFO, Object, 13},
                    {PLANET_INFO, Mesh, 13},
                    {PLANET_INFO, Fragment, 13}
                },
                {
                    {CAMERA_POSITION, Object, 25},
                    {VIEW_MATRIX, Object, 26},
                    {VIEW_MATRIX, Mesh, 26},
                    {PROJECTION_MATRIX, Object, 27},
                    {PROJECTION_MATRIX, Mesh, 27},
                    {VIEW_PROJECTION_MATRIX, Mesh, 28},
                    {SHADOW_DATA, Fragment, 23},
                    {POINT_SHADOW_DATA, Fragment, 24},
                    {ROUGHNESS, Fragment, 25},
                    {METALLIC, Fragment, 26},
                    {CAMERA_POSITION, Fragment, 27},
                    {LIGHTS, Fragment, 28}
                },
                {
                        {Core::BSplineTexture, Object, 2},
                        {Core::BSplineTexture, Mesh, 2},
                        {Core::BSplineTexture, Fragment, 2},
                        {Core::NormalMap, Object, 3},
                        {Core::NormalMap, Mesh, 3},
                        {Core::NormalMap, Fragment, 3},
                        {Core::RockyNoise_TL, Object, 5},
                        {Core::RockyNoise_TL, Mesh, 5},
                        {Core::RockyNoise_TL, Fragment, 5},
                        {Core::RockyNoise_TR, Object, 6},
                        {Core::RockyNoise_TR, Mesh, 6},
                        {Core::RockyNoise_TR, Fragment, 6},
                        {Core::RockyNoise_BL, Object, 7},
                        {Core::RockyNoise_BL, Mesh, 7},
                        {Core::RockyNoise_BL, Fragment, 7},
                        {Core::RockyNoise_BR, Object, 8},
                        {Core::RockyNoise_BR, Mesh, 8},
                        {Core::RockyNoise_BR, Fragment, 8}
                },
                {
                    {Core::ShadowMap, Fragment, 0},
                    {Core::CubeShadowMap, Fragment, 1}
                },
                {
                    {Core::Repeat, Core::ClampToEdge, Core::None, Core::Linear, Core::Linear, true, Fragment, 0},
                    {Core::Repeat, Core::ClampToEdge, Core::None, Core::Linear, Core::Linear, true, Object, 0},
                    {Core::Repeat, Core::ClampToEdge, Core::None, Core::Linear, Core::Linear, true, Mesh, 0},
                    {Core::ClampToEdge, Core::ClampToEdge, Core::None, Core::Linear, Core::Linear, true, Fragment, 1},
                    {Core::ClampToEdge, Core::ClampToEdge, Core::None, Core::Linear, Core::Linear, true, Object, 1},
                    {Core::ClampToEdge, Core::ClampToEdge, Core::None, Core::Linear, Core::Linear, true, Mesh, 1}
                }
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("PlanetDebug"),
            PSOConfig
            {
                "PlanetDebug",
                PipelineType::Mesh,
                "", // No object shader
                "planet_debug_mesh_shader",
                "dynamic_icosphere_fragment_shader",
                Triangle,
                Solid,
                None,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("P"), // Dummy
                {
                    {MODEL_MATRIX, Mesh, 22},
                    {PLANET_CP, Mesh, 10},
                    {PLANET_CP, Fragment, 10},
                    {PLANET_KNOTS_U, Mesh, 11},
                    {PLANET_KNOTS_U, Fragment, 11},
                    {PLANET_KNOTS_V, Mesh, 12},
                    {PLANET_KNOTS_V, Fragment, 12},
                    {PLANET_INFO, Mesh, 13}
                },
                {
                    {VIEW_MATRIX, Mesh, 26},
                    {PROJECTION_MATRIX, Mesh, 27},
                    {SHADOW_DATA, Fragment, 23},
                    {POINT_SHADOW_DATA, Fragment, 24},
                    {ROUGHNESS, Fragment, 25},
                    {METALLIC, Fragment, 26},
                    {CAMERA_POSITION, Fragment, 27},
                    {LIGHTS, Fragment, 28},
                    {PLANET_INFO, Fragment, 13}
                },
                {
                        {Core::BSplineTexture, Fragment, 2},
                        {Core::NormalMap, Fragment, 3},
                        {Core::RockyNoise_TL, Fragment, 5},
                        {Core::RockyNoise_TR, Fragment, 6},
                        {Core::RockyNoise_BL, Fragment, 7},
                        {Core::RockyNoise_BR, Fragment, 8}
                },
                {
                    {Core::ShadowMap, Fragment, 0},
                    {Core::CubeShadowMap, Fragment, 1}
                },
                {
                    {Core::Repeat, Core::Repeat, Core::None, Core::Linear, Core::Linear, true, Fragment, 0},
                    {Core::Repeat, Core::Repeat, Core::None, Core::Linear, Core::Linear, true, Fragment, 1}
                }
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("RockyTextureGenerator"),
            PSOConfig
            {
                "RockyTextureGenerator",
                PipelineType::Compute,
                "generateRockyTexture",
                "", "",
                Triangle,
                Solid,
                None,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("P"),
                {
                    {PLANET_INFO, Vertex, 0},
                    {PLANET_CP, Vertex, 10},
                    {PLANET_KNOTS_U, Vertex, 11},
                    {PLANET_KNOTS_V, Vertex, 12}
                },
                {},
                {
                    {Core::RockyNoise_TL, Fragment, 0},
                    {Core::RockyNoise_TR, Fragment, 1},
                    {Core::RockyNoise_BL, Fragment, 2},
                    {Core::RockyNoise_BR, Fragment, 3}
                },
                {},
                {
                    {Core::Repeat, Core::ClampToEdge, Core::None, Core::Linear, Core::Linear, true, Fragment, 0}
                }
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("PlanetShader"),
            PSOConfig
            {
                "PlanetShader",
                PipelineType::Mesh,
                "planet_object_shader", // Object shader
                "planet_mesh_shader",   // Mesh shader
                "planet_fragment_shader", // Fragment shader
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("P"), // Dummy
                {
                    {MODEL_MATRIX, Object, 22},
                    {MODEL_MATRIX, Mesh, 22},
                    {COMPACT_PLANET_INFO, Object, 13},
                    {COMPACT_PLANET_INFO, Mesh, 13},
                    {COMPACT_PLANET_INFO, Fragment, 13},
                    {BVH_NODES, Fragment, 14},
                    {BVH_PRIMITIVES, Fragment, 15},
                },
                {
                    {CAMERA_POSITION, Object, 25},
                    {VIEW_MATRIX, Object, 26},
                    {PROJECTION_MATRIX, Object, 27},
                    {VIEW_PROJECTION_MATRIX, Mesh, 28},
                    {PREVIOUS_VIEW_PROJECTION_MATRIX, Mesh, 29},
                    {JITTERED_VIEW_PROJECTION_MATRIX, Mesh, 30},
                    {SHADOW_DATA, Fragment, 23},
                    {POINT_SHADOW_DATA, Fragment, 24},
                    {ROUGHNESS, Fragment, 25},
                    {METALLIC, Fragment, 26},
                    {CAMERA_POSITION, Fragment, 27},
                    {LIGHTS, Fragment, 28},
                    {POTENTIAL_SAMPLING_INFO, Fragment, 16},
                {ATMOSPHERE_SETTINGS, Fragment, 21}
                },
                {
                        {Core::BSplineTexture, Object, 2},
                        {Core::BSplineTexture, Mesh, 2},
                        {Core::BSplineTexture, Fragment, 2},
                        {Core::NormalMap, Object, 3},
                        {Core::NormalMap, Mesh, 3},
                        {Core::NormalMap, Fragment, 3}
                },
                {
                    {Core::ShadowMap, Fragment, 0},
                    {Core::CubeShadowMap, Fragment, 1},
                    {Core::AtmosphereDensity3D, Fragment, 4},
                    {Core::LightTransmittance3D, Fragment, 5}

                },
                {
                    {Core::Repeat, Core::ClampToEdge, Core::None, Core::Linear, Core::Linear, true, Fragment, 0},
                    {Core::ClampToEdge, Core::ClampToEdge, Core::ClampToEdge, Core::Linear, Core::Linear, true, Fragment, 1},
                    {Core::Repeat, Core::ClampToEdge, Core::None, Core::Linear, Core::Linear, true, Object, 0},
                    {Core::Repeat, Core::ClampToEdge, Core::None, Core::Linear, Core::Linear, true, Mesh, 0}
                }
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("Billboard"),
            PSOConfig
            {
                "Billboard",
                PipelineType::Vertex,
                "billboardVertexFunction",
                "billboardFragmentFunction",
                "",
                Triangle,
                Solid,
                None,
                Enabled,
                Additive,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PUV"),
                {
                    {BILLBOARD_DATA, Vertex, 13},
                    {BILLBOARD_DATA, Fragment, 0},
                    {MODEL_MATRIX, Vertex, 30},
                    {PARTICLE_SOFTNESS, Fragment, 2}
                },
                {
                    {VIEW_MATRIX, Vertex, 11},
                    {PROJECTION_MATRIX, Vertex, 12},
                    {CAMERA_POSITION, Vertex, 10},
                    {CAMERA_PLANES, Fragment, 1}
                },
                {
                    {Core::Billboard, Fragment, 0}
                },
                {
                    {Core::OpaqueDepth, Fragment, 1}
                },
                {
                    {Core::Repeat, Core::ClampToEdge, Core::None, Core::Linear, Core::Linear, true, Fragment, 0}
                }
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("RayleighShader"),
            PSOConfig
            {
                "RayleighShader",
                PipelineType::Vertex,
                "rayleighVertex",
                "rayleighFragment",
                "",
                Triangle,
                Solid,
                Front,
                Enabled,
                Additive,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PN"),
                {
                        {MODEL_MATRIX, Vertex, 10},
                },
                {
                        {VIEW_PROJECTION_MATRIX, Vertex, 11},
                        {CAMERA_POSITION, Fragment, 10},
                    {SUN_DIRECTION, Fragment, 11},
                    {POTENTIAL_OCTREE, Fragment, 12},
                    {POTENTIAL_OCTREE_INFO, Fragment, 13}
                },
                {},
                {},
                {}
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("Flame"),
            PSOConfig
            {
                "Flame",
                PipelineType::Vertex,
                "vertexFlame",
                "fragmentFlame",
                "",
                Triangle,
                Solid,
                None,
                Enabled,
                Additive,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PCNUV"),
                {
                                {PREVIOUS_MODEL_MATRIX, Vertex, 25},
                                {WARD_ALPHA, Fragment, 22},
                                {ROUGHNESS, Fragment, 25},
                                {METALLIC, Fragment, 26},
                                {NORMAL_MATRIX, Vertex, 26},
                                {MODEL_MATRIX, Vertex, 27},
                                },
                            {
                                {SHADOW_DATA, Fragment, 23},
                                {POINT_SHADOW_DATA, Fragment, 24},
                                {VIEW_PROJECTION_MATRIX, Vertex, 28},
                                {PREVIOUS_VIEW_PROJECTION_MATRIX, Vertex, 29},
                                {JITTERED_VIEW_PROJECTION_MATRIX, Vertex, 30},
                                {CAMERA_POSITION, Fragment, 27},
                                {LIGHTS, Fragment, 28},
                                {TIME, Fragment, 29},
                                {THURST, Fragment, 30}
                            },
                                {
                                }, {
                                    {Core::ShadowMap, Fragment, 0},
                                    {Core::CubeShadowMap, Fragment, 1}
                                },
                                {
                                        {Core::MirrorRepeat,
                                        Core::MirrorRepeat,
                                            Core::None,
                                        Core::Nearest,
                                        Core::Nearest,
                                        true,
                                        Fragment, 0
                                        }
                                }
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("RUSTY_METAL"),
            PSOConfig
            {
                "RUSTY_METAL",
                PipelineType::Vertex,
                "rustyVertex",
                "rustyFragment",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PCNUV"),
                {
                                {PREVIOUS_MODEL_MATRIX, Vertex, 25},
                                {WARD_ALPHA, Fragment, 22},
                                {ROUGHNESS, Fragment, 25},
                                {METALLIC, Fragment, 26},
                                {NORMAL_MATRIX, Vertex, 26},
                                {MODEL_MATRIX, Vertex, 27},
                                },
                            {
                                {SHADOW_DATA, Fragment, 23},
                                {POINT_SHADOW_DATA, Fragment, 24},
                                {VIEW_PROJECTION_MATRIX, Vertex, 28},
                                {PREVIOUS_VIEW_PROJECTION_MATRIX, Vertex, 29},
                                {JITTERED_VIEW_PROJECTION_MATRIX, Vertex, 30},
                                {CAMERA_POSITION, Fragment, 27},
                                {LIGHTS, Fragment, 28}
                            },
                                {
                                }, {
                                    {Core::ShadowMap, Fragment, 0},
                                    {Core::CubeShadowMap, Fragment, 1}
                                },
                                {
                                        {Core::MirrorRepeat,
                                        Core::MirrorRepeat,
                                            Core::None,
                                        Core::Nearest,
                                        Core::Nearest,
                                        true,
                                        Fragment, 0
                                        }
                                }
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("HotMetal"),
            PSOConfig
            {
                "HotMetal",
                PipelineType::Vertex,
                "hotMetalVertex",
                "hotMetalFragment",
                "",
                Triangle,
                Solid,
                Back,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("PUV"),
                {
                        {PREVIOUS_MODEL_MATRIX, Vertex, 25},
                        {MODEL_MATRIX, Vertex, 27},
                }, {
                    {VIEW_PROJECTION_MATRIX, Vertex, 28},
                        {PREVIOUS_VIEW_PROJECTION_MATRIX, Vertex, 29},
                        {JITTERED_VIEW_PROJECTION_MATRIX, Vertex, 30},
                        {HEAT, Fragment, 10},
                        {TIME, Fragment, 11}
                }, {}, {}, {}
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("Sharpening"),
            PSOConfig
            {
                "Sharpening",
                PipelineType::Compute,
                "sharpening_pass",
                "", "",
                Triangle,
                Solid,
                None,
                Enabled,
                Default,
                ColorPixelFormat::BGRAUnorm,
                DepthPixelFormat::Depth32Float,
                G_getVertexDescriptors().at("P"),
                {},
                {},
                {},
                {},
                {}
            }
        },
    };
}
