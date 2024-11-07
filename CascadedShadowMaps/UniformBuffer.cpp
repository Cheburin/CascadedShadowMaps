#include "DXUT.h"

#include <map>
#include <algorithm>
#include <vector>
#include <array>
#include <memory>
#include <assert.h>
#include <malloc.h>
#include <Exception>

#include "uniformbuffer.h"

namespace framework
{

	D3D11_BUFFER_DESC UniformBuffer::getDefaultConstant(unsigned int size)
	{
		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = size;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		return desc;
	}

	D3D11_BUFFER_DESC UniformBuffer::getDefaultStructured(unsigned int size, unsigned int structsize)
	{
		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = size * structsize;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = structsize;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		return desc;
	}

	UniformBuffer::UniformBuffer() :
		m_buffer(0),
		m_isChanged(false),
		m_checkSizeOnSet(true)
	{
	}

	UniformBuffer::~UniformBuffer()
	{
		destroy();
	}

	void UniformBuffer::initBuffer(ID3D11Device* device, size_t elemSize, size_t count, bool createOnCPU)
	{
		//const Device& device = Application::instance()->getDevice();

		HRESULT hr = device->CreateBuffer(&m_desc, 0, &m_buffer);
		if (hr != S_OK)
		{
			//utils::Logger::toLog("Error: could not create an uniform buffer.\n");
			return;
		}

		if (createOnCPU)
		{
			m_bufferInMemory.resize(elemSize * count);
			memset(m_bufferInMemory.data(), 0, m_bufferInMemory.size());
		}

		if (isStructured())
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC svdesc = ResourceView::getDefaultShaderDesc();
			svdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			svdesc.Buffer.FirstElement = 0;
			svdesc.Buffer.NumElements = count;
			m_view.setShaderDesc(svdesc);

			D3D11_UNORDERED_ACCESS_VIEW_DESC uavdesc = ResourceView::getDefaultUAVDesc();
			uavdesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavdesc.Buffer.FirstElement = 0;
			uavdesc.Buffer.Flags = 0;
			uavdesc.Buffer.NumElements = count;
			m_view.setUnorderedAccessDesc(uavdesc);

			m_view.init(device, m_buffer, m_desc.BindFlags);
		}
	}

	void UniformBuffer::initBufferImmutable(ID3D11Device* device, unsigned char* dataPtr, size_t elemSize, size_t count)
	{
		//const Device& device = Application::instance()->getDevice();

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = dataPtr;
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;

		HRESULT hr = device->CreateBuffer(&m_desc, &data, &m_buffer);
		if (hr != S_OK)
		{
			//utils::Logger::toLog("Error: could not create an uniform immutable buffer.\n");
			return;
		}

		if (isStructured())
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC svdesc = ResourceView::getDefaultShaderDesc();
			svdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			svdesc.Buffer.FirstElement = 0;
			svdesc.Buffer.NumElements = count;
			m_view.setShaderDesc(svdesc);
		}
	}

	void UniformBuffer::destroy()
	{
		m_view.destroy();

		if (m_buffer != 0)
		{
			m_buffer->Release();
			m_buffer = 0;
		}
		m_bufferInMemory.clear();
		m_isChanged = false;
	}

	void UniformBuffer::applyChanges(ID3D11DeviceContext* context)
	{
		if (m_bufferInMemory.empty()) return;
		if (m_isChanged)
		{
			//const Device& device = Application::instance()->getDevice();

			D3D11_MAPPED_SUBRESOURCE data;
			data.pData = NULL;
			data.DepthPitch = data.RowPitch = 0;

			D3D11_MAP mapType = D3D11_MAP_WRITE;
			if ((m_desc.Usage & D3D11_USAGE_DYNAMIC) != 0 && (m_desc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) != 0)
			{
				mapType = D3D11_MAP_WRITE_DISCARD;
			}

			HRESULT hr = context->Map(m_buffer, 0, mapType, 0, &data);
			if (hr != S_OK)
			{
				//utils::Logger::toLog("Error: could not apply changes to an uniform buffer.\n");
				return;
			}
			memcpy(data.pData, m_bufferInMemory.data(), m_bufferInMemory.size());
			context->Unmap(m_buffer, 0);

			m_isChanged = false;
		}
	}

	unsigned int UniformBuffer::getElementByteSize() const
	{
		if (isStructured()) return m_desc.StructureByteStride;
		return m_desc.ByteWidth;
	}

	bool UniformBuffer::isStructured() const
	{
		return (m_desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) != 0;
	}

}