// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <cassert>
#include <v8.h>

namespace napa {
namespace module {

    // Base class C++ class wrapper to manage lifetime.
    // It comes from node_object_wrap.h.
    class ObjectWrap {
    public:

        ObjectWrap() {
            _refs = 0;
        }

        virtual ~ObjectWrap() {
            if (persistent().IsEmpty()) {
                return;
            }

            assert(persistent().IsNearDeath());
            persistent().ClearWeak();
            persistent().Reset();
        }

        template <class T>
        static inline T* Unwrap(v8::Local<v8::Object> handle) {
            assert(!handle.IsEmpty());
            assert(handle->InternalFieldCount() > 0);

            // Cast to ObjectWrap before casting to T.  A direct cast from void
            // to T won't work right when T has more than one base class.
            void* ptr = handle->GetAlignedPointerFromInternalField(0);
            ObjectWrap* wrap = static_cast<ObjectWrap*>(ptr);
            return static_cast<T*>(wrap);
        }

        v8::Local<v8::Object> handle() {
            return handle(v8::Isolate::GetCurrent());
        }

        v8::Local<v8::Object> handle(v8::Isolate* isolate) {
            return v8::Local<v8::Object>::New(isolate, persistent());
        }

        v8::Persistent<v8::Object>& persistent() {
            return _handle;
        }

    protected:

        void Wrap(v8::Local<v8::Object> handle) {
            assert(persistent().IsEmpty());
            assert(handle->InternalFieldCount() > 0);
            handle->SetAlignedPointerInInternalField(0, this);
            persistent().Reset(v8::Isolate::GetCurrent(), handle);
            MakeWeak();
        }

        void MakeWeak(void) {
            persistent().SetWeak(this, WeakCallback, v8::WeakCallbackType::kParameter);
            persistent().MarkIndependent();
        }

        /* Ref() marks the object as being attached to an event loop.
        * Refed objects will not be garbage collected, even if
        * all references are lost.
        */
        virtual void Ref() {
            assert(!persistent().IsEmpty());
            persistent().ClearWeak();
            _refs++;
        }

        /* Unref() marks an object as detached from the event loop.  This is its
        * default state.  When an object with a "weak" reference changes from
        * attached to detached state it will be freed. Be careful not to access
        * the object after making this call as it might be gone!
        * (A "weak reference" means an object that only has a
        * persistant handle.)
        *
        * DO NOT CALL THIS FROM DESTRUCTOR
        */
        virtual void Unref() {
            assert(!persistent().IsEmpty());
            assert(!persistent().IsWeak());
            assert(_refs > 0);
            if (--_refs == 0) {
                MakeWeak();
            }
        }

        // Reference counter.
        int _refs;

    private:

        static void WeakCallback(const v8::WeakCallbackInfo<ObjectWrap>& data) {
            ObjectWrap* wrap = data.GetParameter();
            assert(wrap->_refs == 0);
            wrap->_handle.Reset();
            delete wrap;
        }

        v8::Persistent<v8::Object> _handle;
    };

}   // End of namespace module.
}   // End of namespace napa.