template<class T>
struct KListEntry
{
	KListEntry * pPrev;
	KListEntry * pNext;
	// no destructor necessary
	void Init()
	{
		pPrev = this;
		pNext = this;
	}
	KListEntry()
	{
		Init();
	}
	void InsertHead(T * entry)
	{
		__assume(NULL != entry);
		static_cast<KListEntry<T> *>(entry)->pPrev = this;
		static_cast<KListEntry<T> *>(entry)->pNext = pNext;
		pNext->pPrev = static_cast<KListEntry<T> *>(entry);
		pNext = static_cast<KListEntry<T> *>(entry);
	}
	void InsertTail(T * entry)
	{
		__assume(NULL != entry);
		static_cast<KListEntry<T> *>(entry)->pNext = this;
		static_cast<KListEntry<T> *>(entry)->pPrev = pPrev;
		pPrev->pNext = static_cast<KListEntry<T> *>(entry);
		pPrev = static_cast<KListEntry<T> *>(entry);
	}
	bool IsEmpty() const
	{
		return pNext == this;
	}
	T * RemoveHead()
	{
		KListEntry * tmp = pNext;
		tmp->pNext->pPrev = this;
		pNext = tmp->pNext;
		tmp->Init();
		__assume(NULL != tmp);
		return static_cast<T *>(tmp);
	}
	T * RemoveTail()
	{
		KListEntry * tmp = pPrev;
		tmp->pPrev->pNext = this;
		pPrev = tmp->pPrev;
		tmp->Init();
		__assume(NULL != tmp);
		return static_cast<T *>(tmp);
	}
	void RemoveFromList()
	{
		pNext->pPrev = pPrev;
		pPrev->pNext = pNext;
		Init();
	}
	static void RemoveEntry(T * entry)
	{
		__assume(NULL != entry);
		static_cast<KListEntry<T> *>(entry)->pNext->pPrev = static_cast<KListEntry<T> *>(entry)->pPrev;
		static_cast<KListEntry<T> *>(entry)->pPrev->pNext = static_cast<KListEntry<T> *>(entry)->pNext;
		static_cast<KListEntry<T> *>(entry)->Init();
	}

	// move all the list to DstList. The list becomes empty
	void RemoveAll(KListEntry & DstList)
	{
		if ( ! IsEmpty())
		{
			KListEntry * pListEntry = pNext;
			RemoveFromList();
			Init();
			pListEntry->InsertTailList( & DstList);
		}
		else
		{
			DstList.Init();
		}
	}

	T * Next() const { __assume(NULL != pNext); return static_cast<T *>(pNext); }
	T * Prev() const { __assume(NULL != pPrev); return static_cast<T *>(pPrev); }

	// call a function with any return type
	template <class F> void CallForEach(F function)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			(p->*function)();
		}
	}
	// call a function with any return and argument type
	template <class F, class A> void CallForEach(F function, A a)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			(p->*function)(a);
		}
	}
	template <class F> NTSTATUS CallForEachWhileNotSuccess(F function)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			NTSTATUS status = (p->*function)();
			if (STATUS_SUCCESS == status)
			{
				return STATUS_SUCCESS;
			}
		}
		return STATUS_UNSUCCESSFUL;
	}
	template <class F, class A> NTSTATUS CallForEachWhileNotSuccess(F function, A a)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			NTSTATUS status = (p->*function)(a);
			if (STATUS_SUCCESS == status)
			{
				return STATUS_SUCCESS;
			}
		}
		return STATUS_UNSUCCESSFUL;
	}
	template <class F> NTSTATUS CallForEachWhileSuccess(F function)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			NTSTATUS status = (p->*function)();
			if (STATUS_SUCCESS != status)
			{
				return status;
			}
		}
		return STATUS_SUCCESS;
	}
	template <class F, class A> NTSTATUS CallForEachWhileSuccess(F function, A a)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			NTSTATUS status = (p->*function)(a);
			if (STATUS_SUCCESS != status)
			{
				return status;
			}
		}
		return STATUS_SUCCESS;
	}
	template <class F> bool CallForEachWhileTrue(F function)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			if ( ! (p->*function)())
			{
				return false;
			}
		}
		return true;
	}
	template <class F, class A> bool CallForEachWhileTrue(F function, A a)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			if ( ! (p->*function)(a))
			{
				return false;
			}
		}
		return true;
	}
	template <class F> bool CallForEachWhileNotTrue(F function)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			if ((p->*function)())
			{
				return true;
			}
		}
		return false;
	}
	template <class F, class A> bool CallForEachWhileNotTrue(F function, A a)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			if ((p->*function)(a))
			{
				return true;
			}
		}
		return false;
	}

};

template<class T >
struct LockedListHead: public KListEntry<T>, public CSimpleCriticalSection
{
	// no destructor necessary
	void InsertHead(T * entry)
	{
		Lock();
		KListEntry::InsertHead(entry);
		Unlock();
	}
	void InsertHeadUnsafe(T * entry)
	{
		Lock();
		KListEntry::InsertHead(entry);
	}
	void InsertTail(T * entry)
	{
		Lock();
		KListEntry::InsertTail(entry);
		Unlock();
	}
	void InsertTailUnsafe(T * entry)
	{
		KListEntry::InsertTail(entry);
	}
	T * RemoveHead()
	{
		CSimpleCriticalSectionLock lock(*this);
		if (IsEmpty())
		{
			return NULL;
		}
		return KListEntry::RemoveHead();
	}
	T * RemoveHeadUnsafe()
	{
		return KListEntry::RemoveHead();
	}
	void RemoveEntry(T * Entry)
	{
		CSimpleCriticalSectionLock lock(*this);
		KListEntry::RemoveEntry(Entry);
	}
	static void RemoveEntryUnsafe(T * entry)
	{
		KListEntry::RemoveEntry(Entry);
	}

	// move all the list to DstList. The list becomes empty
	void RemoveAll(KListEntry & DstList)
	{
		Lock();
		KListEntry::RemoveAll(DstList);
		Unlock();
	}

	// call a function with any return type
	template <class F> void CallForEach(F function)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			(p->*function)();
		}
	}
	// call a function with any return and argument type
	template <class F, class A> void CallForEach(F function, A a)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			(p->*function)(a);
		}
	}
	template <class F> NTSTATUS CallForEachWhileNotSuccess(F function)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			NTSTATUS status = (p->*function)();
			if (STATUS_SUCCESS == status)
			{
				return STATUS_SUCCESS;
			}
		}
		return STATUS_UNSUCCESSFUL;
	}
	template <class F, class A> NTSTATUS CallForEachWhileNotSuccess(F function, A a)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			NTSTATUS status = (p->*function)(a);
			if (STATUS_SUCCESS == status)
			{
				return STATUS_SUCCESS;
			}
		}
		return STATUS_UNSUCCESSFUL;
	}
	template <class F> NTSTATUS CallForEachWhileSuccess(F function)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			NTSTATUS status = (p->*function)();
			if (STATUS_SUCCESS != status)
			{
				return status;
			}
		}
		return STATUS_SUCCESS;
	}
	template <class F, class A> NTSTATUS CallForEachWhileSuccess(F function, A a)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			NTSTATUS status = (p->*function)(a);
			if (STATUS_SUCCESS != status)
			{
				return status;
			}
		}
		return STATUS_SUCCESS;
	}
	template <class F> bool CallForEachWhileTrue(F function)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			if ( ! (p->*function)())
			{
				return false;
			}
		}
		return true;
	}
	template <class F, class A> bool CallForEachWhileTrue(F function, A a)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			if ( ! (p->*function)(a))
			{
				return false;
			}
		}
		return true;
	}
	template <class F> bool CallForEachWhileNotTrue(F function)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			if ((p->*function)())
			{
				return true;
			}
		}
		return false;
	}
	template <class F, class A> bool CallForEachWhileNotTrue(F function, A a)
	{
		for (T * p = Next(); p != this; p = p->KListEntry<T>::Next())
		{
			if ((p->*function)(a))
			{
				return true;
			}
		}
		return false;
	}
};

