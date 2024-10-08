#pragma once

/* <editor-fold desc="MIT License">

Copyright(c) 2024 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/threading/ActivityStatus.h>

#include <condition_variable>
#include <list>

namespace vsg
{

    /// Thread safe queue deleting nodes/subgraphs as batches, typically done from a background thread.
    class VSG_DECLSPEC DeleteQueue : public Inherit<Object, DeleteQueue>
    {
    public:
        explicit DeleteQueue(ref_ptr<ActivityStatus> status);

        struct ObectToDelete
        {
            uint64_t frameCount = 0;
            ref_ptr<Object> object;
        };

        using ObjectsToDelete = std::list<ObectToDelete>;

        std::atomic_uint64_t frameCount = 0;
        uint64_t retainForFrameCount = 3;

        ActivityStatus* getStatus() { return _status; }
        const ActivityStatus* getStatus() const { return _status; }

        void advance(ref_ptr<FrameStamp> frameStamp);

        void add(ref_ptr<Object> object)
        {
            std::scoped_lock lock(_mutex);
            _objectsToDelete.push_back(ObectToDelete{frameCount + retainForFrameCount, object});
            _cv.notify_one();
        }

        template<typename T>
        void add(T& objects)
        {
            std::scoped_lock lock(_mutex);
            for (auto& object : objects)
            {
                _objectsToDelete.emplace_back(ObectToDelete{frameCount + retainForFrameCount, object});
            }
            _cv.notify_one();
        }

        void wait_then_clear();

        void clear();

    protected:
        virtual ~DeleteQueue();

        std::mutex _mutex;
        std::condition_variable _cv;
        ObjectsToDelete _objectsToDelete;
        ref_ptr<ActivityStatus> _status;
    };
    VSG_type_name(vsg::DeleteQueue);

} // namespace vsg