#pragma once

namespace Heart
{
    enum class ColorFormat
    {
        R8 = 0,
        RG8, RGB8, RGBA8, RGBA32
    };

    enum class MsaaSampleCount
    {
        None = 1,
        Two, Four, Eight, Sixteen, Thirtytwo, Sixtyfour,
        Max = Sixtyfour
    };

    struct FrameBufferCreateInfo
    {
        bool HasDepth = false;
        bool ForceResolve = false; // the framebuffer will automatically have a resolve if SampleCount is > none
        ColorFormat ColorFormat = ColorFormat::RGBA8;
        u32 Width, Height = 0;
        u32 ResolveWidth, ResolveHeight = 0;
        MsaaSampleCount SampleCount = MsaaSampleCount::Max;
    };

    class FrameBuffer
    {
    public:
        FrameBuffer(const FrameBufferCreateInfo& createInfo)
            : m_Info(createInfo)
        {}
        virtual ~FrameBuffer() = default;

    public:
        static Ref<FrameBuffer> Create(const FrameBufferCreateInfo& createInfo);

    private:
        FrameBufferCreateInfo m_Info;
    };
}