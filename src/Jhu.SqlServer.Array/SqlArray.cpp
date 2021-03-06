#include "stdafx.h"
#include "macro.h"
#include "SqlArray.h"
#include "SqlArrayCultureFormatter.h"

namespace Jhu { namespace SqlServer { namespace Array
{
	
#pragma region Properties

	// This will be specialized at the end of the file
	template <class T, class B>
	inline typename B::DataTypeT SqlArray<T, B>::DataType::get()
	{
		throw gcnew NotImplementedException();	//
	}

	template <class T, class B>
	inline typename B::IndexT SqlArray<T, B>::TotalLength::get()
	{
		return m_length;
	}

#pragma endregion
#pragma region Constructors

	template <class T, class B>
	SqlArray<T, B>::SqlArray(PARLIST_1(typename B::IndexT, i))
	{
		if (i0 <= 0)
		{
			throw gcnew SqlArrayException(ExceptionMessages::GetString(L"InvalidLength"));
		}

		AllocBuffer(VARLIST_1(i));
		WriteSignature();
		WriteLengths(VARLIST_1(i));
	}

	template <class T, class B>
	SqlArray<T, B>::SqlArray(PARLIST_2(typename B::IndexT, i))
	{
		if (i0 <= 0 || i1 <= 0)
		{
			throw gcnew SqlArrayException(ExceptionMessages::GetString(L"InvalidLength"));
		}

		AllocBuffer(VARLIST_2(i));
		WriteSignature();
		WriteLengths(VARLIST_2(i));
	}

	template <class T, class B>
	SqlArray<T, B>::SqlArray(array<typename B::IndexT>^ length)
	{
		if (length == nullptr || length->Length == 0 || length->Length > 6)
		{
			throw gcnew SqlArrayException(ExceptionMessages::GetString(L"InvalidLength"));
		}

		AllocBuffer(length);
		WriteSignature();
		WriteLengths(length);
	}

	template <class T, class B>
	SqlArray<T, B>::SqlArray(typename B::SqlBufferT data)
	{
		InitBuffer(B::GetBuffer(data, ArrayLoadMethod::AllData));
	}

	template <class T, class B>
	SqlArray<T, B>::SqlArray(typename B::SqlBufferT data, ArrayLoadMethod method)
	{
		InitBuffer(B::GetBuffer(data, method));
	}

#pragma endregion

	template <class T, class B>
	inline interior_ptr<typename B::HeaderT> SqlArray<T, B>::GetHeaderPointer()
	{
		return GET_BUFFER_T(B::HeaderT, m_buffer, B::GetHeaderOffset());
	}

	template <class T, class B>
	inline interior_ptr<typename B::IndexT> SqlArray<T, B>::GetLengthsPointer()
	{
		return GET_BUFFER_T(B::IndexT, m_buffer, B::GetLengthsOffset());
	}

	template <class T, class B>
	inline interior_ptr<T> SqlArray<T, B>::GetDataPointer()
	{
		return GET_BUFFER_T(T, m_buffer, B::GetDataOffset(m_rank));
	}

	template <class T, class B>
	inline typename B::IndexT SqlArray<T, B>::GetLengthsOffset()
	{
		return B::GetLengthsOffset();
	}

	template <class T, class B>
	inline typename B::IndexT SqlArray<T, B>::GetDataOffset()
	{
		return B::GetDataOffset(m_rank);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::InitBuffer(array<typename B::BufferT>^ buffer)
	{
		m_buffer = buffer;
		ReadSignature();
	}

	template <class T, class B>
	inline void SqlArray<T, B>::CastBuffer(array<typename B::BufferT>^ buffer, typename B::IndexT offset)
	{
		Buffer::BlockCopy(buffer, offset, m_buffer, GetDataOffset(), m_length * sizeof(T));
	}

	template <class T, class B>
	inline void SqlArray<T, B>::AllocBuffer(array<typename B::IndexT>^ length)
	{
		m_rank = length->Length;
		m_length = GetTotalLength(length);
		m_buffer = gcnew array<B::BufferT>(B::GetBufferSize(m_rank, m_length, sizeof(T)));
	}


	template <class T, class B>
	inline void SqlArray<T, B>::AllocBuffer(PARLIST_1(typename B::IndexT, length))
	{
		m_rank = 1;
		m_length = length0;
		m_buffer = gcnew array<B::BufferT>(B::GetBufferSize(m_rank, m_length, sizeof(T)));
	}

	template <class T, class B>
	inline void SqlArray<T, B>::AllocBuffer(PARLIST_2(typename B::IndexT, length))
	{
		m_rank = 2;
		m_length = length0 * length1;
		m_buffer = gcnew array<B::BufferT>(B::GetBufferSize(m_rank, m_length, sizeof(T)));
	}

	// ---

	template <class T, class B>
	inline void SqlArray<T, B>::ReadSignature()
	{
		interior_ptr<B::HeaderT> hp = GetHeaderPointer();
		
		// Verify data type
		if (DataType != hp->DataType || hp->HeaderType != B::HeaderType)
		{
			throw gcnew SqlArrayException(ExceptionMessages::GetString(L"TypeMismatch"));
		}
		
		m_rank = hp->Rank;
		m_length = hp->Length;
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteSignature()
	{
		interior_ptr<B::HeaderT> hp = GetHeaderPointer();

		hp->HeaderType = B::HeaderType;

		hp->DataType = DataType;
		hp->Rank = m_rank;
		hp->Length = m_length;
	}

	// ---

	template <class T, class B>
	inline void SqlArray<T, B>::WriteLengths(array<typename B::IndexT>^ lengths)
	{
		Buffer::BlockCopy(lengths, 0, m_buffer, GetLengthsOffset(), lengths->Length * sizeof(B::IndexT));
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteLengths(PARLIST_1(typename B::IndexT, i))
	{
		interior_ptr<B::IndexT> rp = GetLengthsPointer();
		LAYOUTLIST_1(rp, i);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteLengths(PARLIST_2(typename B::IndexT, i))
	{
		interior_ptr<B::IndexT> rp = GetLengthsPointer();
		LAYOUTLIST_2(rp, i);
	}

	template <class T, class B>
	inline bool SqlArray<T, B>::IterateCounter(array<typename B::IndexT>^ counter, array<typename B::IndexT>^ length, typename B::RankT rank)
	{
		bool more = false;

		for (B::RankT r = rank; r < counter->Length; r++)
		{
			if (counter[r] < length[r] - 1)
			{
				counter[r]++;
				more = true;
				break;
			}
			else
			{
				if (r == m_rank - 1)
				{
					more = false;
					break;
				}
				else
				{
					counter[r] = 0;
				}
			}
		}

		return more;
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteData(array<T>^ data)
	{
		Buffer::BlockCopy(data, 0, m_buffer, GetDataOffset(), data->Length * sizeof(T));
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteData(PARLIST_1(T, i))
	{
		interior_ptr<T> p = GetDataPointer();
		LAYOUTLIST_1(p, i);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteData(PARLIST_2(T, i))
	{
		interior_ptr<T> p = GetDataPointer();
		LAYOUTLIST_2(p, i);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteData(PARLIST_3(T, i))
	{
		interior_ptr<T> p = GetDataPointer();
		LAYOUTLIST_3(p, i);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteData(PARLIST_4(T, i))
	{
		interior_ptr<T> p = GetDataPointer();
		LAYOUTLIST_4(p, i);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteData(PARLIST_5(T, i))
	{
		interior_ptr<T> p = GetDataPointer();
		LAYOUTLIST_5(p, i);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteData(PARLIST_6(T, i))
	{
		interior_ptr<T> p = GetDataPointer();
		LAYOUTLIST_6(p, i);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteData(PARLIST_7(T, i))
	{
		interior_ptr<T> p = GetDataPointer();
		LAYOUTLIST_7(p, i);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteData(PARLIST_8(T, i))
	{
		interior_ptr<T> p = GetDataPointer();
		LAYOUTLIST_8(p, i);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteData(PARLIST_9(T, i))
	{
		interior_ptr<T> p = GetDataPointer();
		LAYOUTLIST_9(p, i);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::WriteData(PARLIST_10(T, i))
	{
		interior_ptr<T> p = GetDataPointer();
		LAYOUTLIST_10(p, i);
	}

	template <class T, class B>
	typename B::IndexT SqlArray<T, B>::GetTotalLength(array<typename B::IndexT> ^length)
	{
		B::IndexT total = 1;
		for (int i = 0; i < length->Length; i ++)
		{
			total *= length[i];
		}

		return total;
	}

	template <class T, class B>
	inline void SqlArray<T, B>::LoadData(typename B::SqlBufferT data, array<typename B::IndexT>^ start, array<typename B::IndexT>^ end)
	{
		B::IndexT si = (B::IndexT)(GetDataOffset() + GetLinearIndex(start) * sizeof(T));
		B::IndexT ei = (B::IndexT)(GetDataOffset() + (1 + GetLinearIndex(end)) * sizeof(T));
		B::LoadBuffer(data, m_buffer, si, ei);
	}

	template <class T, class B>
	inline void SqlArray<T, B>::LoadData(typename B::SqlBufferT data, typename B::IndexT start)
	{
		start = (B::IndexT)(GetDataOffset() + start * sizeof(T));
		B::LoadBuffer(data, m_buffer, start, (B::IndexT)(start + sizeof(T)));
	}

	// ---

		template <class T, class B>
	inline typename B::IndexT SqlArray<T, B>::GetLinearIndex(array<typename B::IndexT> ^indexes)
	{
		// assumes column major ordering of data
		// row, column, page etc
		interior_ptr<B::IndexT> lp = GetLengthsPointer();

		B::IndexT index = 0;
		B::IndexT page = 1;
		for (int i = 0; i < indexes->Length; i ++)
		{
			index += indexes[i] * page;
			page *= lp[i];
		}

		return index;
	}

	template <class T, class B>
	inline typename B::IndexT SqlArray<T, B>::GetLinearIndex(PARLIST_1(typename B::IndexT, i))
	{
		B::IndexT index;
		interior_ptr<B::IndexT> rp = GetLengthsPointer();

		index = i0;

		return index;
	}

	template <class T, class B>
	inline typename B::IndexT SqlArray<T, B>::GetLinearIndex(PARLIST_2(typename B::IndexT, i))
	{
		B::IndexT index;
		interior_ptr<B::IndexT> rp = GetLengthsPointer();

		index = i0;
		index += i1 * rp[0];

		return index;
	}

	template <class T, class B>
	inline typename B::IndexT SqlArray<T, B>::GetLinearIndex(PARLIST_3(typename B::IndexT, i))
	{
		B::IndexT index;
		interior_ptr<B::IndexT> rp = GetLengthsPointer();

		index = i0;
		index += i1 * rp[0];
		index += i2 * rp[0] * rp[1];

		return index;
	}

	template <class T, class B>
	inline typename B::IndexT SqlArray<T, B>::GetLinearIndex(PARLIST_4(typename B::IndexT, i))
	{
		B::IndexT index, page;
		interior_ptr<B::IndexT> rp = GetLengthsPointer();

		index = i0; page = rp[0];
		index += i1 * page; page *= rp[1];
		index += i2 * page; page *= rp[2];
		index += i3 * page;

		return index;
	}

	template <class T, class B>
	inline typename B::IndexT SqlArray<T, B>::GetLinearIndex(PARLIST_5(typename B::IndexT, i))
	{
		B::IndexT index, page;
		interior_ptr<B::IndexT> rp = GetLengthsPointer();

		index = i0; page = rp[0];
		index += i1 * page; page *= rp[1];
		index += i2 * page; page *= rp[2];
		index += i3 * page; page *= rp[3];
		index += i4 * page;

		return index;
	}

	template <class T, class B>
	inline typename B::IndexT SqlArray<T, B>::GetLinearIndex(PARLIST_6(typename B::IndexT, i))
	{
		B::IndexT index, page;
		interior_ptr<B::IndexT> rp = GetLengthsPointer();

		index = i0; page = rp[0];
		index += i1 * page; page *= rp[1];
		index += i2 * page; page *= rp[2];
		index += i3 * page; page *= rp[3];
		index += i4 * page; page *= rp[4];
		index += i5 * page;

		return index;
	}

	// ---

	template <class T, class B>
	interior_ptr<T> SqlArray<T, B>::ToString_Subarray(StringBuilder^ sb, interior_ptr<typename B::IndexT> lp, interior_ptr<T> dp, typename B::RankT r, CultureInfo^ culture)
	{
		sb->Append(L"[");

		if (r == 0)
		{
			for (B::IndexT i = 0; i < lp[r]; i ++)
			{
				if (i > 0)
				{
					sb->Append(culture->TextInfo->ListSeparator);
				}

				sb->Append(SqlArrayCultureFormatter::ToString(*(dp++), culture));
			}
		}
		else
		{
			for (B::IndexT i = 0; i < lp[r]; i ++)
			{
				if (i > 0)
				{
					sb->Append(culture->TextInfo->ListSeparator);
				}
				dp = ToString_Subarray(sb, lp, dp, r - 1, culture);
			}
		}

		sb->Append(L']');

		return dp;
	}

	template <class T, class B>
	void SqlArray<T, B>::FromArray_Subarray(typename B::IndexT% p, typename B::RankT rank, array<typename B::IndexT>^ length, System::Array^ data)
	{
		if (rank == m_rank - 1)
		{
			B::IndexT count = length[rank] * (B::IndexT)sizeof(T);
			Buffer::BlockCopy(data, 0, m_buffer, p, length[rank] * sizeof(T));
			p += count;
		}
		else
		{
			for (B::IndexT i = 0; i < length[rank]; i ++)
			{
				FromArray_Subarray(p, rank + 1, length, safe_cast<System::Array^>(data->GetValue(i)));
			}
		}
	}

	// ---

	template <class T, class B>
	T SqlArray<T, B>::default::get(PARLIST_1(typename B::IndexT, i))
	{
		return GetDataPointer()[GetLinearIndex(VARLIST_1(i))];
	}

	template <class T, class B>
	void SqlArray<T, B>::default::set(PARLIST_1(typename B::IndexT, i), T value)
	{
		GetDataPointer()[GetLinearIndex(VARLIST_1(i))] = value;
	}

	template <class T, class B>
	T SqlArray<T, B>::default::get(PARLIST_2(typename B::IndexT, i))
	{
		return GetDataPointer()[GetLinearIndex(VARLIST_2(i))];
	}

	template <class T, class B>
	void SqlArray<T, B>::default::set(PARLIST_2(typename B::IndexT, i), T value)
	{
		GetDataPointer()[GetLinearIndex(VARLIST_2(i))] = value;
	}

	template <class T, class B>
	T SqlArray<T, B>::default::get(PARLIST_3(typename B::IndexT, i))
	{
		return GetDataPointer()[GetLinearIndex(VARLIST_3(i))];
	}

	template <class T, class B>
	void SqlArray<T, B>::default::set(PARLIST_3(typename B::IndexT, i), T value)
	{
		GetDataPointer()[GetLinearIndex(VARLIST_3(i))] = value;
	}

	template <class T, class B>
	T SqlArray<T, B>::default::get(PARLIST_4(typename B::IndexT, i))
	{
		return GetDataPointer()[GetLinearIndex(VARLIST_4(i))];
	}

	template <class T, class B>
	void SqlArray<T, B>::default::set(PARLIST_4(typename B::IndexT, i), T value)
	{
		GetDataPointer()[GetLinearIndex(VARLIST_4(i))] = value;
	}

	template <class T, class B>
	T SqlArray<T, B>::default::get(PARLIST_5(typename B::IndexT, i))
	{
		return GetDataPointer()[GetLinearIndex(VARLIST_5(i))];
	}

	template <class T, class B>
	void SqlArray<T, B>::default::set(PARLIST_5(typename B::IndexT, i), T value)
	{
		GetDataPointer()[GetLinearIndex(VARLIST_5(i))] = value;
	}

	template <class T, class B>
	T SqlArray<T, B>::default::get(PARLIST_6(typename B::IndexT, i))
	{
		return GetDataPointer()[GetLinearIndex(VARLIST_6(i))];
	}

	template <class T, class B>
	void SqlArray<T, B>::default::set(PARLIST_6(typename B::IndexT, i), T value)
	{
		GetDataPointer()[GetLinearIndex(VARLIST_6(i))] = value;
	}

	template <class T, class B>
	T SqlArray<T, B>::default::get(array<typename B::IndexT>^ i)
	{
		return GetDataPointer()[GetLinearIndex(i)];
	}

	template <class T, class B>
	void SqlArray<T, B>::default::set(array<typename B::IndexT>^ i, T value)
	{
		GetDataPointer()[GetLinearIndex(i)] = value;
	}

	// ---

	template <class T, class B>
	void SqlArray<T, B>::Read(System::IO::BinaryReader ^r)
	{
		InitBuffer(r->ReadBytes((int)r->BaseStream->Length));
	}

	template <class T, class B>
	void SqlArray<T, B>::Write(System::IO::BinaryWriter ^w)
	{
		w->Write(m_buffer);
	}

	template <class T, class B>
	inline typename B::SqlBufferT SqlArray<T, B>::ToSqlBuffer()
	{
		return B::ToSqlBuffer(m_buffer);
	}

	template <class T, class B>
	array<T>^ SqlArray<T, B>::ToArray()
	{
		array<T>^ a = gcnew array<T>(m_length);
		Buffer::BlockCopy(m_buffer, GetDataOffset(), a, 0, m_length * sizeof(T));
		return a;
	}

	template <class T, class B>
	array<typename B::IndexT>^ SqlArray<T, B>::GetLengths()
	{
		array<B::IndexT>^ a = gcnew array<B::IndexT>(m_rank);
		Buffer::BlockCopy(m_buffer, B::GetLengthsOffset(), a, 0, m_rank * sizeof(B::IndexT));
		return a;
	}


	template <class T, class B>
	String^ SqlArray<T, B>::ToString()
	{
		return ToString(Globalization::CultureInfo::CurrentCulture);
	}

	template <class T, class B>
	String^ SqlArray<T, B>::ToString(CultureInfo^ culture)
	{
		StringBuilder^ sb = gcnew StringBuilder();

		interior_ptr<B::IndexT> lp = GetLengthsPointer();
		interior_ptr<T> dp = GetDataPointer();

		ToString_Subarray(sb, lp, dp, m_rank - 1, culture);

		return sb->ToString();
	}

	template <class T, class B>
	System::Array^ SqlArray<T, B>::ToJaggedArray()
	{
		interior_ptr<B::IndexT> lp = GetLengthsPointer();
		B::IndexT dp = GetDataOffset();

		// Index order will be reversed, start from highest rank
		return ToJaggedArray_Subarray(lp, dp, m_rank - 1);
	}

	template <class T, class B>
	System::Array^ SqlArray<T, B>::ToJaggedArray_Subarray(interior_ptr<typename B::IndexT> lp, typename B::IndexT% dp, typename B::RankT r)
	{
		if (r == 0)
		{
			array<T>^ a = gcnew array<T>(lp[r]);
			B::IndexT count = (B::IndexT)(a->Length * sizeof(T));
			Buffer::BlockCopy(m_buffer, dp, a, 0, count);
			dp += count;
			return a;
		}
		else
		{
			System::Array^ a = nullptr;

			for (B::IndexT i = 0; i < lp[r]; i ++)
			{
				System::Array^ b = ToJaggedArray_Subarray(lp, dp, r - 1);

				if (i == 0)
				{
					a = safe_cast<System::Array^>(Activator::CreateInstance(b->GetType()->MakeArrayType(), gcnew array<Object^> { lp[r] }));
				}

				a->SetValue(b, i);
			}

			return a;
		}
	}

	template <class T, class B>
	System::Array^ SqlArray<T, B>::ToMultiDArray()
	{
		System::Type^ t = T::typeid->MakeArrayType(m_rank);
		
		array<B::IndexT>^ length = GetLengths();
		array<Object^>^ par = gcnew array<Object^>(length->Length);

		// Reverse order of indices
		for (typename B::RankT i = 0; i < length->Length; i++)
		{
			par[par->Length - 1 - i] = length[i];
		}

		System::Array^ r = safe_cast<System::Array^>(Activator::CreateInstance(t, par));
		Buffer::BlockCopy(m_buffer, GetDataOffset(), r, 0, m_length * sizeof(T));

		return r;
	}

	template <class T, class B>
	bool SqlArray<T, B>::IsSameShape(SqlArray<T, B> other)
	{
		// Check array compatibility
		array<B::IndexT>^ al = GetLengths();
		array<B::IndexT>^ bl = other.GetLengths();

		if (al->Length != bl->Length)
		{
			return false;
		}

		for (B::RankT i = 0; i < al->Length; i ++)
		{
			if (al[i] != bl[i])
			{
				return false;
			}
		}

		return true;
	}

	template <class T, class B>
	void SqlArray<T, B>::Reshape(array<typename B::IndexT>^ length)
	{
		// Do compatibility check
		if (GetTotalLength(length) != m_length)
		{
			throw gcnew SqlArrayException(ExceptionMessages::GetString(L"IncompatibleShape"));
		}

		// Check if rank is the same
		if (length->Length == m_rank)
		{
			// No buffer copy needed, overwrite lengths
			WriteLengths(length);
		}
		else
		{
			// Rank differs, change buffer size and do buffer copy
			array<B::BufferT>^ oldbuffer = m_buffer;
			B::IndexT olddataoffset = GetDataOffset();

			AllocBuffer(length);
			WriteSignature();
			WriteLengths(length);

			// Copy old data
			Buffer::BlockCopy(oldbuffer, olddataoffset, m_buffer, GetDataOffset(), m_length * sizeof(T));
		}
	}

	template <class T, class B>
	SqlArray<T, B> SqlArray<T, B>::Transpose()
	{
		if (m_rank != 2)
		{
			throw gcnew Exception(L"Only two-index arrays can be transposed");	// ******
		}

		array<B::IndexT>^ length = GetLengths();
		array<B::IndexT>^ newlength = gcnew array<B::IndexT>(m_rank);
		newlength[0] = length[1];
		newlength[1] = length[0];

		SqlArray<T, B> u(newlength);

		interior_ptr<T> sp = GetDataPointer();
		interior_ptr<T> dp = u.GetDataPointer();

		B::IndexT li = length[0];
		B::IndexT lj = length[1];

		for (B::IndexT i = 0; i < li; i ++)
		{
			for (B::IndexT j = 0; j < lj; j ++)
			{
				dp[j + i * lj] = sp[i + j * li];
			}
		}

		return u;
	}

	template <class T, class B>
	SqlArray<T, B> SqlArray<T, B>::Permute(array<typename B::RankT>^ indices)
	{
		if (indices->Length != m_rank)
		{
			throw gcnew ArgumentException();	//*****
		}

		// Verify that indices are listed only once, also
		// put the new length array together
		array<B::IndexT>^ length = GetLengths();
		array<bool>^ check = gcnew array<bool>(m_rank);
		array<B::IndexT>^ newlength = gcnew array<B::IndexT>(m_rank);

		for (B::RankT r = 0; r < check->Length; r ++)
		{
			if (check[indices[r]])
			{
				// Index listed twice
				throw gcnew SqlArrayException();	//*****
			}
			else
			{
				check[indices[r]] = true;
			}

			newlength[r] = length[indices[r]];
		}

		// *** Code below needs optimization
		
		// Create new buffer and copy header
		SqlArray<T, B> u(newlength);

		interior_ptr<T> sp = GetDataPointer();
		interior_ptr<T> dp = u.GetDataPointer();

		array<B::IndexT>^ current = gcnew array<B::IndexT>(m_rank);
		array<B::IndexT>^ mapped = gcnew array<B::IndexT>(m_rank);
		bool exit = false;

		while (!exit)
		{
			// Calculate source index
			for (B::RankT r = 0; r < mapped->Length; r++)
			{
				mapped[r] = current[indices[r]];
			}

			dp[u.GetLinearIndex(mapped)] = *sp++;

			for (B::RankT r = 0; r < current->Length; r++)
			{
				if (current[r] < length[r] - 1)
				{
					current[r]++;
					break;
				}
				else
				{
					if (r == m_rank - 1)
					{
						exit = true;
						break;
					}
					else
					{
						current[r] = 0;
					}
				}
			}
		}

		return u;
	}

	template <class T, class B>
	SqlArray<T, B> SqlArray<T, B>::GetItems(array<array<typename B::IndexT>^>^ indices)
	{
		interior_ptr<T> sp = GetDataPointer();

		SqlArray<T, B> u(indices->Length);
		interior_ptr<T> dp = u.GetDataPointer();

		for (B::IndexT i = 0; i < indices->Length; i++)
		{
			*dp++ = sp[GetLinearIndex(indices[i])];
		}

		return u;
	}

	template <class T, class B>
	void SqlArray<T, B>::SetItems(array<array<typename B::IndexT>^>^ indices, array<typename T>^ values)
	{
		interior_ptr<T> dp = GetDataPointer();

		for (B::IndexT i = 0; i < indices->Length; i++)
		{
			dp[GetLinearIndex(indices[i])] = values[i];
		}
	}

	template <class T, class B>
	SqlArray<T, B> SqlArray<T, B>::GetSubarray(array<typename B::IndexT>^ from, array<typename B::IndexT>^ size, bool collapse)
	{
		array<B::IndexT>^ length = GetLengths();

		// Check ranges and compute new length
		List<B::IndexT>^ newlengthlist = gcnew List<B::IndexT>(length->Length);
		
		array<B::IndexT>^ jump = gcnew array<B::IndexT>(length->Length);
		B::IndexT jj = 1;

		bool cons = true;
		typename B::RankT hcr = 0;		// highest consecutive rank
		B::IndexT hcl = 1;	// highest consecutive block length
		
		for (typename B::RankT i = 0; i < length->Length; i ++)
		{
			if (from[i] < 0 || from[i] + size[i] > length[i])
			{
				throw gcnew IndexOutOfRangeException();	// **** 
			}
			else if (size[i] < 1)
			{
				throw gcnew ArgumentException();	//****
			}
			else // if (from[i] <= to[i])
			{		
				if (size[i] > 1 || !collapse)
				{
					newlengthlist->Add(size[i]);
				}

				// Calculate consecutive block size
				if (cons)
				{
					hcr = i + 1;
					hcl *= size[i];
				}
				cons &= (from[i] == 0 && size[i] == length[i]);
			}

			jump[i] = jj;
			jj *= length[i];
		}

		array<B::IndexT>^ newlength = newlengthlist->ToArray();
		SqlArray<T, B> u(newlength);

		array<B::IndexT>^ current = gcnew array<B::IndexT>(from->Length);
		from->CopyTo(current, 0);

		interior_ptr<T> sp = GetDataPointer();
		interior_ptr<T> dp = u.GetDataPointer();

		B::IndexT spos = GetLinearIndex(current);
		B::IndexT dpos = 0;
		B::IndexT soff = GetDataOffset();
		B::IndexT doff = u.GetDataOffset();


		bool exit = false;
		while (!exit)
		{
			// Copy consecutive block item
			
			// Depending on the highest consecutive length, byte per byte copy
			// could be faster than BlockCopy.
			//*** figure out when to do byte per byte copy and when buffer copy
			if (hcl == 1)
			{
				*dp++ = sp[spos];
			}
			else
			{
				//*** can the *s be avoided?
				Buffer::BlockCopy(m_buffer,
					soff + spos * sizeof(T),
					u.m_buffer, doff + dpos * sizeof(T), hcl * sizeof(T));
				dpos += hcl;
			}

			if (hcr == current->Length)
			{
				exit = true;
				break;
			}
			else
			{
				for (B::RankT r = hcr; r < current->Length; r++)
				{
					if (current[r] < from[r] + size[r] - 1)
					{
						current[r]++;
						spos += jump[r];
						break;
					}
					else
					{
						if (r == m_rank - 1)
						{
							exit = true;
							break;
						}
						else
						{
							current[r] = from[r];
							spos -= (size[r] - 1) * jump[r];
						}
					}
				}
			}
		}

		return u;
	}

	template <class T, class B>
	SqlArray<T, B> SqlArray<T, B>::GetSubarrays(array<array<typename B::IndexT>^>^ from, array<typename B::IndexT>^ size, bool collapse)
	{
		// Count non-zero length
		B::RankT nonzero = 0;
		for (B::RankT i = 0; i < size->Length; i++)
		{
			if (size[i] > 1 || size[i] == 1 && !collapse)
			{
				nonzero++;
			}
		}

		// Create results array size array
		array<B::IndexT>^ nlen = gcnew array<B::IndexT>(nonzero + 1);
		B::RankT q = 1;

		nlen[0] = from->Length;
		for (B::RankT i = 0; i < size->Length; i++)
		{
			if (size[i] > 1 || size[i] == 1 && !collapse)
			{
				nlen[q++] = size[i];
			}
		}

		// Create new offset array
		array<B::IndexT>^ nix = gcnew array<B::IndexT>(nonzero + 1);

		// Create results array
		nlen[0] = from->Length;
		SqlArray<T, B> r(nlen);

		// Reshape subarrays to fit into results array
		nlen[0] = 1;

		for (B::IndexT i = 0; i < from->Length; i ++)
		{
			SqlArray<T, B> sa = GetSubarray(from[i], size, collapse);
			sa.Reshape(nlen);

			nix[0] = i;
			r.SetSubarray(nix, sa);
		}

		return r;
	}

	template <class T, class B>
	void SqlArray<T, B>::SetSubarray(array<typename B::IndexT>^ from, SqlArray<T, B> value)
	{
		array<B::IndexT>^ length = GetLengths();
		array<B::IndexT>^ size = value.GetLengths();


		// Calculate maximum copy stride
		array<B::IndexT>^ jump = gcnew array<B::IndexT>(length->Length);
		B::IndexT jj = 1;

		bool cons = true;
		typename B::RankT hcr = 0;		// highest consecutive rank
		B::IndexT hcl = 1;	// highest consecutive block length
		
		for (typename B::RankT i = 0; i < length->Length; i ++)
		{
			if (from[i] < 0 || from[i] + size[i] > length[i])
			{
				throw gcnew IndexOutOfRangeException();	// **** 
			}
			else if (size[i] < 1)
			{
				throw gcnew ArgumentException();	//****
			}
			else // if (from[i] <= to[i])
			{		
				// Calculate consecutive block size
				if (cons)
				{
					hcr = i + 1;
					hcl *= size[i];
				}
				cons &= (from[i] == 0 && size[i] == length[i]);
			}

			jump[i] = jj;
			jj *= length[i];
		}


		// Initialize loop variable
		array<B::IndexT>^ current = gcnew array<B::IndexT>(from->Length);
		from->CopyTo(current, 0);

		interior_ptr<T> dp = GetDataPointer();
		interior_ptr<T> sp = value.GetDataPointer();

		B::IndexT dpos = GetLinearIndex(current);
		B::IndexT spos = 0;
		B::IndexT doff = GetDataOffset();
		B::IndexT soff = value.GetDataOffset();


		bool exit = false;
		while (!exit)
		{
			// Copy consecutive block item
			
			// Depending on the highest consecutive length, byte per byte copy
			// could be faster than BlockCopy.
			//*** figure out when to do byte per byte copy and when buffer copy
			if (hcl == 1)
			{
				dp[dpos] = *sp++;
			}
			else
			{
				//*** can the *s be avoided?
				//Buffer::BlockCopy(m_buffer,
				//	soff + spos * sizeof(T),
				//	u.m_buffer, doff + dpos * sizeof(T), hcl * sizeof(T));
				
				Buffer::BlockCopy(
					value.m_buffer, soff + spos * sizeof(T),
					m_buffer, doff + dpos * sizeof(T),
					hcl * sizeof(T));

				spos += hcl;
			}

			if (hcr == current->Length)
			{
				exit = true;
				break;
			}
			else
			{
				for (B::RankT r = hcr; r < current->Length; r++)
				{
					if (current[r] < from[r] + size[r] - 1)
					{
						current[r]++;
						dpos += jump[r];
						break;
					}
					else
					{
						if (r == m_rank - 1)
						{
							exit = true;
							break;
						}
						else
						{
							current[r] = from[r];
							dpos -= (size[r] - 1) * jump[r];
						}
					}
				}
			}
		}
	}

	template <class T, class B>
	void SqlArray<T, B>::SetSubarrays(array<array<typename B::IndexT>^>^ from, SqlArray<T, B> value)
	{
		array<typename B::IndexT>^ olen = value.GetLengths();

		// Subarray size as to get from values
		array<B::IndexT>^ slen = gcnew array<B::IndexT>(olen->Length - 1);
		for (B::RankT i = 0; i < slen -> Length; i ++)
		{
			slen[i] = olen[i + 1];
		}

		olen[0] = 1;

		// Source index
		array<B::IndexT>^ off = gcnew array<B::IndexT>(value.m_rank);

		for (B::IndexT i = 0; i < from->Length; i ++)
		{
			off[0] = i;
			SqlArray<T, B> sa = value.GetSubarray(off, olen, false);
			sa.Reshape(slen);
			this->SetSubarray(from[i], sa);
		}
	}

	// ---

	template <class T, class B>
	typename SqlArray<T, B> SqlArray<T, B>::TensorProduct(SqlArray<T, B> a, SqlArray<T, B> b)
	{
		SqlArray<T, B> aa(a);
		SqlArray<T, B> ba(b);

		// Make sure a and b are vectors of the same size
		if (aa.m_rank != 1 || ba.m_rank != 1)
		{
			throw gcnew SqlArrayException("Not vectors of the same size");	//******
		}

		SqlArray<T, B> res(gcnew array<B::IndexT>(2) { *aa.GetLengthsPointer(), *ba.GetLengthsPointer() });

		/*interior_ptr<T> pa = aa.GetDataPointer();
		interior_ptr<T> pb = ba.GetDataPointer();
		interior_ptr<T> pr = res.GetDataPointer();*/

		// ****** optimize to use pointers

		for (B::IndexT i = 0; i < aa.m_length; i ++)
		{
			for (B::IndexT j = 0; j < ba.m_length; j ++)
			{
				res[i, j] = aa[i] * ba[j];
			}
		}

		return res;
	}

	template <class T, class B>
	SqlArray<T, B> SqlArray<T, B>::InnerProduct(SqlArray<T, B> a, SqlArray<T, B> b)
	{
		array<B::IndexT>^ alen = a.GetLengths();
		array<B::IndexT>^ blen = b.GetLengths();

		// Check compatibility
		if (alen[0] != blen[0])
		{
			throw gcnew ArgumentException("Indices are not compatible.");
		}

		// Make sure that result is not a scalar (dot product of vectors)
		if (alen->Length + blen->Length <= 2)
		{
			throw gcnew ArgumentException("Result is scalar, use function DotProduct instead.");
		}

		// New size comes from the non summing indices
		array<B::IndexT>^ nlen = gcnew array<B::IndexT>(alen->Length + blen->Length - 2);
		
		B::IndexT ni = 0;
		for (B::IndexT i = 1; i < alen->Length; i ++)
		{
			nlen[ni++] = alen[i];
		}
		for (B::IndexT i = 1; i < blen->Length; i ++)
		{
			nlen[ni++] = blen[i];
		}

		SqlArray<T, B> u(nlen);

		B::IndexT ca = 0;
		for (B::RankT i = 1; i < alen->Length; i ++)
		{
			ca += alen[i];
		}

		B::IndexT cb = 0;
		for (B::RankT i = 1; i < blen->Length; i ++)
		{
			cb += blen[i];
		}

		interior_ptr<T> pa = a.GetDataPointer();
		interior_ptr<T> pb = b.GetDataPointer();
		interior_ptr<T> pu = u.GetDataPointer();

		interior_ptr<T> ppa, ppb;

		B::IndexT ib = 0;
		do
		{
			ppa = pa;


			B::IndexT ia = 0;
			do
			{
				ppb = pb;

				// Inner loop to do the row by column product
				T sum = T(0);
				for (B::IndexT i = 0; i < alen[0]; i ++)
				{
					sum += *ppa++ * *ppb++;
				}
				
				*pu++ = sum;

				ia ++;

			} while (ia < ca);

			pb = ppb;
			ib ++;

		} while (ib < cb);

		return u;
	}

	// ---

	template <class T, class B>
	inline array<typename B::IndexT>^ SqlArray<T, B>::GetTotalLengthFromArray(System::Array^ data)
	{
		// Reverse order of indices

		array<B::IndexT>^ r = gcnew array<B::IndexT>(data->Rank);

		for (int i = 0; i < data->Rank; i++)
		{
			r[i] = data->GetLength(data->Rank - i - 1);
		}

		return r;
	}

	template <class T, class B>
	SqlArray<T, B> SqlArray<T, B>::FromArray(System::Array^ data)
	{
		// Check array type
		System::Type^ t = data->GetType();

		if (t->IsArray && t->GetElementType()->IsValueType)
		{
			// 1D or multi-D array

			if (t->GetElementType() != T::typeid)
			{
				throw gcnew SqlArrayException(ExceptionMessages::GetString(L"TypeMismatch"));
			}

			SqlArray<T, B> u(GetTotalLengthFromArray(data));
			Buffer::BlockCopy(data, 0, u.m_buffer, u.GetDataOffset(), data->Length * sizeof(T));

			return u;
		}
		else
		{
			// Jagged array

			// Determine rank
			int rank = 0;
			while (t->IsArray)
			{
				rank++;
				t = t->GetElementType();
			}

			if (t != T::typeid)
			{
				throw gcnew SqlArrayException(ExceptionMessages::GetString(L"TypeMismatch"));
			}

			array<B::IndexT>^ length = gcnew array<B::IndexT>(rank);

			System::Array^ a = data;
			for (int i = 0; i < length->Length; i ++)
			{
				length[i] = a->Length;

				if (i < rank - 1)
				{
					a = safe_cast<System::Array^>(a->GetValue(0));
				}
			}
			
			SqlArray<T, B> u(length);
			B::IndexT p = u.GetDataOffset();
			u.FromArray_Subarray(p, 0, length, data);

			return u;
		}
	}

	template <class T, class B>
	SqlArray<T, B> SqlArray<T, B>::FromString(String^ data)
	{
		return FromString(data, Globalization::CultureInfo::CurrentCulture);
	}

	template <class T, class B>
	SqlArray<T, B> SqlArray<T, B>::FromString(String^ data, CultureInfo^ culture)
	{
		ArrayParser^ p = gcnew ArrayParser(culture->TextInfo->ListSeparator, culture->NumberFormat->NumberDecimalSeparator);

		array<int>^ length;
		array<Token>^ tokens;
		p->Parse(data, length, tokens);

		array<B::IndexT>^ length2 = gcnew array<B::IndexT>(length->Length);
		for (int i = 0; i < length2->Length; i ++)
		{
			length2[i] = (B::IndexT)length[i];
		}

		SqlArray<T, B> u(length2);
		interior_ptr<T> d = u.GetDataPointer();
		for (int i = 0; i < tokens->Length; i ++)
		{
			 *d++ = SqlArrayCultureFormatter::Parse<T>(data->Substring(tokens[i].Position, tokens[i].Length), culture);
		}

		return u;
	}

	template <class T, class B>
	SqlArray<T, B> SqlArray<T, B>::Zeros(array<typename B::IndexT>^ length)
	{
		SqlArray<T, B> u = SqlArray<T, B>(length);
		interior_ptr<T> dp = u.GetDataPointer();

		B::IndexT tlen = GetTotalLength(length);
		for (B::IndexT i = 0; i < tlen; i++)
		{
			dp[i] = T();
		}

		return u;
	}

	//---

	// ---- Template specializations

#define SPECIALIZE_SQLARRAY(T, DATATYPEID) \
	template <>	inline ShortArray::DataTypeT SqlArray<T, ShortArray>::DataType::get() { return DATATYPEID; } \
	template <>	inline MaxArray::DataTypeT SqlArray<T, MaxArray>::DataType::get() { return DATATYPEID; } \
	template SqlArray<T, ShortArray>; \
	template SqlArray<T, MaxArray>;

	SPECIALIZE_SQLARRAY(Byte, DATATYPEID_BYTE)
	SPECIALIZE_SQLARRAY(Int16, DATATYPEID_INT16)
	SPECIALIZE_SQLARRAY(Int32, DATATYPEID_INT32)
	SPECIALIZE_SQLARRAY(Int64, DATATYPEID_INT64)
	SPECIALIZE_SQLARRAY(Single, DATATYPEID_SINGLE)
	SPECIALIZE_SQLARRAY(Double, DATATYPEID_DOUBLE)
	SPECIALIZE_SQLARRAY(SqlComplex<Single>, DATATYPEID_SINGLE)
	SPECIALIZE_SQLARRAY(SqlComplex<Double>, DATATYPEID_DOUBLE)

} } }