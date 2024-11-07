#pragma once
#include "resourceview.h"
namespace framework
{

	class UniformBuffer
	{

	public:
		UniformBuffer();
		virtual ~UniformBuffer();

		static D3D11_BUFFER_DESC getDefaultConstant(unsigned int size);
		static D3D11_BUFFER_DESC getDefaultStructured(unsigned int size, unsigned int structsize);

		template<typename DataType> bool initImmutable(const std::vector<DataType>& data, const D3D11_BUFFER_DESC& desc)
		{
			destroy();
			m_desc = desc;
			m_desc.Usage = D3D11_USAGE_IMMUTABLE;
			m_desc.CPUAccessFlags = 0;
			initBufferImmutable(data.data(), sizeof(DataType), count);

			//if (m_buffer != 0) initDestroyable();
			return m_buffer != 0;
		}

		template<typename DataType> bool initDefaultConstant()
		{
			return init<DataType>(1, getDefaultConstant(sizeof(DataType)));
		}

		template<typename DataType> bool initDefaultStructured(ID3D11Device* device, size_t count)
		{
			return init<DataType>(device, count, getDefaultStructured(count, sizeof(DataType)));
		}

		template<typename DataType> bool init(ID3D11Device* device, size_t count, const D3D11_BUFFER_DESC& desc)
		{
			destroy();
			m_desc = desc;
			initBuffer(device, sizeof(DataType), count, true);

			//if (m_buffer != 0) initDestroyable();
			return m_buffer != 0;
		}

		template<typename DataType> const DataType& getData() const
		{
			return getElement<DataType>(0);
		}

		template<typename DataType> const DataType& getElement(int index) const
		{
			static DataType dummy;
			size_t i = index * sizeof(DataType);
			if (i < 0 || i >= m_bufferInMemory.size()) return dummy;
			return reinterpret_cast<const DataType&>(m_bufferInMemory[i]);
		}

		template<typename DataType> void setData(const DataType& value)
		{
			setElement<DataType>(0, value);
		}

		template<typename DataType> void setElement(int index, const DataType& value)
		{
			size_t i = index * sizeof(DataType);
			if (i < 0 || i >= m_bufferInMemory.size()) return;
			DataType& elem = reinterpret_cast<DataType&>(m_bufferInMemory[i]);
			elem = value;
			m_isChanged = true;
		}

		template<typename DataType> int size() const
		{
			return m_bufferInMemory.size() / sizeof(DataType);
		}

		unsigned int getElementByteSize() const;
		const D3D11_BUFFER_DESC& getDesc() const { return m_desc; }
		ID3D11Buffer* getBuffer() { return m_buffer; }
		ResourceView& getView() { return m_view; }
		bool isStructured() const;
		bool checkSizeOnSet() const { return m_checkSizeOnSet; }

		void applyChanges(ID3D11DeviceContext* context);

	protected:
		virtual void destroy();
		void initBuffer(ID3D11Device* device, size_t elemSize, size_t count, bool createOnCPU);
		void initBufferImmutable(ID3D11Device* device, unsigned char* dataPtr, size_t elemSize, size_t count);

		ID3D11Buffer* m_buffer;
		D3D11_BUFFER_DESC m_desc;
		ResourceView m_view;

		std::vector<unsigned char> m_bufferInMemory;
		bool m_isChanged;
		bool m_checkSizeOnSet;
	};

}
