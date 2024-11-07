#pragma once

namespace framework
{
	class ResourceView
	{
		friend class UniformBuffer;
		friend class UnorderedAccessBuffer;

		static D3D11_SHADER_RESOURCE_VIEW_DESC getDefaultShaderDesc();
		static D3D11_RENDER_TARGET_VIEW_DESC getDefaultRenderTargetDesc();
		static D3D11_DEPTH_STENCIL_VIEW_DESC getDefaultDepthStencilDesc();
		static D3D11_UNORDERED_ACCESS_VIEW_DESC getDefaultUAVDesc();
		static D3D11_SHADER_RESOURCE_VIEW_DESC getTexture2DShaderDesc(int arraySize, bool msaa);
		static D3D11_RENDER_TARGET_VIEW_DESC getTexture2DRenderTargetDesc(int arraySize, bool msaa);
		static D3D11_DEPTH_STENCIL_VIEW_DESC getTexture2DDepthStencilDesc(int arraySize, bool msaa);
		static D3D11_UNORDERED_ACCESS_VIEW_DESC getTexture2DUAVDesc(int arraySize);

		void setShaderDesc(const D3D11_SHADER_RESOURCE_VIEW_DESC& desc);
		void setRenderTargetDesc(const D3D11_RENDER_TARGET_VIEW_DESC& desc);
		void setDepthStencilDesc(const D3D11_DEPTH_STENCIL_VIEW_DESC& desc);
		void setUnorderedAccessDesc(const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc);
		void init(ID3D11Device* device, ID3D11Resource* resource, unsigned int bindFlags);
		void destroy();
		bool isValid() const;

	public:
		ResourceView();
		~ResourceView();

		ID3D11ShaderResourceView* asShaderView() const { return m_shaderView; }
		ID3D11RenderTargetView* asRenderTargetView() const { return m_renderTargetView; }
		ID3D11DepthStencilView* asDepthStencilView() const { return m_depthStencilView; }
		ID3D11UnorderedAccessView* asUAView() const { return m_uaView; }

	private:
		std::pair<D3D11_SHADER_RESOURCE_VIEW_DESC, bool> m_shaderDesc;
		ID3D11ShaderResourceView* m_shaderView;
		std::pair<D3D11_RENDER_TARGET_VIEW_DESC, bool> m_renderTargetDesc;
		ID3D11RenderTargetView* m_renderTargetView;
		std::pair<D3D11_DEPTH_STENCIL_VIEW_DESC, bool> m_depthStencilDesc;
		ID3D11DepthStencilView* m_depthStencilView;
		std::pair<D3D11_UNORDERED_ACCESS_VIEW_DESC, bool> m_uavDesc;
		ID3D11UnorderedAccessView* m_uaView;
	};

}
