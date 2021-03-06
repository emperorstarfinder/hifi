//
//  HighlightEffect.h
//  render-utils/src/
//
//  Created by Olivier Prat on 08/08/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_HighlightEffect_h
#define hifi_render_utils_HighlightEffect_h

#include <render/Engine.h>
#include <render/HighlightStage.h>
#include <render/RenderFetchCullSortTask.h>

#include "DeferredFramebuffer.h"
#include "DeferredFrameTransform.h"

class HighlightRessources {
public:
    HighlightRessources();

    gpu::FramebufferPointer getDepthFramebuffer();
    gpu::TexturePointer getDepthTexture();

    gpu::FramebufferPointer getColorFramebuffer();

    // Update the source framebuffer size which will drive the allocation of all the other resources.
    void update(const gpu::FramebufferPointer& primaryFrameBuffer);
    const glm::ivec2& getSourceFrameSize() const { return _frameSize; }

protected:

    gpu::FramebufferPointer _depthFrameBuffer;
    gpu::FramebufferPointer _colorFrameBuffer;
    gpu::TexturePointer _depthStencilTexture;

    glm::ivec2 _frameSize;

    void allocateColorBuffer(const gpu::FramebufferPointer& primaryFrameBuffer);
    void allocateDepthBuffer(const gpu::FramebufferPointer& primaryFrameBuffer);
};

using HighlightRessourcesPointer = std::shared_ptr<HighlightRessources>;

class HighlightSharedParameters {
public:

    enum {
        MAX_PASS_COUNT = 8
    };

    HighlightSharedParameters();

    std::array<render::HighlightStage::Index, MAX_PASS_COUNT> _highlightIds;

    static float getBlurPixelWidth(const render::HighlightStyle& style, int frameBufferHeight);
};

using HighlightSharedParametersPointer = std::shared_ptr<HighlightSharedParameters>;

class PrepareDrawHighlight {
public:
    using Inputs = gpu::FramebufferPointer;
    using Outputs = HighlightRessourcesPointer;
    using JobModel = render::Job::ModelIO<PrepareDrawHighlight, Inputs, Outputs>;

    PrepareDrawHighlight();

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

private:

    HighlightRessourcesPointer _ressources;

};

class SelectionToHighlight {
public:

    using Outputs = std::vector<std::string>;
    using JobModel = render::Job::ModelO<SelectionToHighlight, Outputs>;

    SelectionToHighlight(HighlightSharedParametersPointer parameters) : _sharedParameters{ parameters } {}

    void run(const render::RenderContextPointer& renderContext, Outputs& outputs);

private:

    HighlightSharedParametersPointer _sharedParameters;
};

class ExtractSelectionName {
public:

    using Inputs = SelectionToHighlight::Outputs;
    using Outputs = std::string;
    using JobModel = render::Job::ModelIO<ExtractSelectionName, Inputs, Outputs>;

    ExtractSelectionName(unsigned int highlightIndex) : _highlightPassIndex{ highlightIndex } {}

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

private:

    unsigned int _highlightPassIndex;

};

class DrawHighlightMask {
public:

    using Inputs = render::VaryingSet3<render::ShapeBounds, HighlightRessourcesPointer, glm::vec2>;
    using Outputs = glm::ivec4;
    using JobModel = render::Job::ModelIO<DrawHighlightMask, Inputs, Outputs>;

    DrawHighlightMask(unsigned int highlightIndex, render::ShapePlumberPointer shapePlumber, HighlightSharedParametersPointer parameters);

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

protected:

    unsigned int _highlightPassIndex;
    render::ShapePlumberPointer _shapePlumber;
    HighlightSharedParametersPointer _sharedParameters;
    gpu::BufferPointer _boundsBuffer;
    gpu::StructBuffer<glm::vec2> _outlineWidth;

    static gpu::PipelinePointer _stencilMaskPipeline;
    static gpu::PipelinePointer _stencilMaskFillPipeline;
};

class DrawHighlight {
public:

    using Inputs = render::VaryingSet5<DeferredFrameTransformPointer, HighlightRessourcesPointer, DeferredFramebufferPointer, glm::ivec4, gpu::FramebufferPointer>;
    using Config = render::Job::Config;
    using JobModel = render::Job::ModelI<DrawHighlight, Inputs, Config>;

    DrawHighlight(unsigned int highlightIndex, HighlightSharedParametersPointer parameters);

    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

#include "Highlight_shared.slh"

    using HighlightConfigurationBuffer = gpu::StructBuffer<HighlightParameters>;

    static const gpu::PipelinePointer& getPipeline(const render::HighlightStyle& style);

    static gpu::PipelinePointer _pipeline;
    static gpu::PipelinePointer _pipelineFilled;

    unsigned int _highlightPassIndex;
    HighlightParameters _parameters;
    HighlightSharedParametersPointer _sharedParameters;
    HighlightConfigurationBuffer _configuration;
};

class DebugHighlightConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(bool viewMask MEMBER viewMask NOTIFY dirty)

public:

    bool viewMask{ false };

signals:
    void dirty();
};

class DebugHighlight {
public:
    using Inputs = render::VaryingSet4<HighlightRessourcesPointer, glm::ivec4, glm::vec2, gpu::FramebufferPointer>;
    using Config = DebugHighlightConfig;
    using JobModel = render::Job::ModelI<DebugHighlight, Inputs, Config>;

    DebugHighlight();
    ~DebugHighlight();

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

private:

    gpu::PipelinePointer _depthPipeline;
    int _geometryDepthId{ 0 };
    bool _isDisplayEnabled{ false };

    const gpu::PipelinePointer& getDepthPipeline();
    void initializePipelines();
};

class DrawHighlightTask {
public:

    using Inputs = render::VaryingSet5<RenderFetchCullSortTask::BucketList, DeferredFramebufferPointer, gpu::FramebufferPointer, DeferredFrameTransformPointer, glm::vec2>;
    using Config = render::Task::Config;
    using JobModel = render::Task::ModelI<DrawHighlightTask, Inputs, Config>;

    DrawHighlightTask();

    void configure(const Config& config);
    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs);

private:
    static const render::Varying addSelectItemJobs(JobModel& task, const render::Varying& selectionName, const RenderFetchCullSortTask::BucketList& items);

};

#endif // hifi_render_utils_HighlightEffect_h


