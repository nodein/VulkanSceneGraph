#pragma once

/* <editor-fold desc="MIT License">

Copyright(c) 2018 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/animation/AnimationManager.h>
#include <vsg/app/CompileManager.h>
#include <vsg/app/Presentation.h>
#include <vsg/app/RecordAndSubmitTask.h>
#include <vsg/app/UpdateOperations.h>
#include <vsg/app/Window.h>
#include <vsg/threading/Barrier.h>
#include <vsg/threading/FrameBlock.h>
#include <vsg/utils/Instrumentation.h>

#include <map>

namespace vsg
{

    /// Viewer provides high level viewer functionality for managing windows, handling events and recording and submitting
    /// command graphs for compute and rendering.
    class VSG_DECLSPEC Viewer : public Inherit<Object, Viewer>
    {
    public:
        Viewer();

        Viewer(const Viewer&) = delete;
        Viewer& operator=(const Viewer& rhs) = delete;

        /// add Window to Viewer
        virtual void addWindow(ref_ptr<Window> window);

        /// remove Window from Viewer
        virtual void removeWindow(ref_ptr<Window> window);

        Windows& windows() { return _windows; }
        const Windows& windows() const { return _windows; }

        clock::time_point& start_point() { return _start_point; }
        const clock::time_point& start_point() const { return _start_point; }

        FrameStamp* getFrameStamp() { return _frameStamp; }
        const FrameStamp* getFrameStamp() const { return _frameStamp; }

        /// return true if viewer is valid and active
        bool active() const;

        /// schedule closure of the viewer and associated windows, after a call to Viewer::close() the Viewer::active() method will return false
        virtual void close();

        /// poll the events for all attached windows, return true if new events are available
        virtual bool pollEvents(bool discardPreviousEvents = true);

        /// get the current set of Events that are filled in by prior calls to pollEvents
        UIEvents& getEvents() { return _events; }

        /// get the const current set of Events that are filled in by prior calls to pollEvents
        const UIEvents& getEvents() const { return _events; }

        /// add event handler
        void addEventHandler(ref_ptr<Visitor> eventHandler) { _eventHandlers.emplace_back(eventHandler); }

        void addEventHandlers(const EventHandlers& eventHandlers) { _eventHandlers.insert(_eventHandlers.end(), eventHandlers.begin(), eventHandlers.end()); }

        /// get the list of EventHandlers
        EventHandlers& getEventHandlers() { return _eventHandlers; }

        /// get the const list of EventHandlers
        const EventHandlers& getEventHandlers() const { return _eventHandlers; }

        /// thread safe container for update operations
        ref_ptr<UpdateOperations> updateOperations;

        /// add an update operation
        void addUpdateOperation(ref_ptr<Operation> op, UpdateOperations::RunBehavior runBehavior = UpdateOperations::ONE_TIME)
        {
            updateOperations->add(op, runBehavior);
        }

        /// manager for starting and running animations
        ref_ptr<AnimationManager> animationManager;

        /// compile manager provides thread safe support for compiling subgraphs
        ref_ptr<CompileManager> compileManager;

        /// hint for setting the FrameStamp::simulationTime to time since start_point()
        static constexpr double UseTimeSinceStartPoint = std::numeric_limits<double>::max();

        /// Convenience method for advancing to the next frame.
        /// Check active status, return false if viewer no longer active.
        /// If still active, poll for pending events and place them in the Events list and advance to the next frame, generate updated FrameStamp to signify the advancement to a new frame and return true.
        virtual bool advanceToNextFrame(double simulationTime = UseTimeSinceStartPoint);

        /// pass the Events into any registered EventHandlers
        virtual void handleEvents();

        virtual void compile(ref_ptr<ResourceHints> hints = {});

        virtual bool acquireNextFrame();

        /// call vkWaitForFence on the fences associated with previous frames RecordAndSubmitTask, a relativeFrameIndex of 1 is the previous frame, 2 is two frames ago.
        /// timeout is in nanoseconds.
        virtual VkResult waitForFences(size_t relativeFrameIndex, uint64_t timeout);

        // Manage the work to do each frame using RecordAndSubmitTasks. Those that need to present results need to be wired up to respective Presentation objects.
        RecordAndSubmitTasks recordAndSubmitTasks;

        // Manage the presentation of rendering using Presentation objects
        using Presentations = std::vector<ref_ptr<Presentation>>;
        Presentations presentations;

        /// Create RecordAndSubmitTask and Presentation objects configured to manage specified commandGraphs and assign them to the viewer.
        /// Replace any preexisting setup.
        virtual void assignRecordAndSubmitTaskAndPresentation(CommandGraphs commandGraphs);

        /// Add command graphs creating RecordAndSubmitTask/Presentation objects where appropriate.
        void addRecordAndSubmitTaskAndPresentation(CommandGraphs commandGraphs);

        ref_ptr<ActivityStatus> status;
        std::list<std::thread> threads;

        void setupThreading();
        void stopThreading();

        virtual void update();

        virtual void recordAndSubmit();

        virtual void present();

        /// Call vkDeviceWaitIdle on all the devices associated with this Viewer
        virtual void deviceWaitIdle() const;

        /// Hook for assigning Instrumentation to enable profiling of record traversal.
        uint64_t frameReference = 0;
        ref_ptr<Instrumentation> instrumentation;

        /// Convenience method for assigning Instrumentation to the viewer and any associated objects.
        void assignInstrumentation(ref_ptr<Instrumentation> in_instrumentation);

    protected:
        virtual ~Viewer();

        bool _close = false;

        Windows _windows;

        bool _firstFrame = true;
        clock::time_point _start_point;
        ref_ptr<FrameStamp> _frameStamp;

        UIEvents _events;
        EventHandlers _eventHandlers;

        bool _threading = false;
        ref_ptr<FrameBlock> _frameBlock;
        ref_ptr<Barrier> _submissionCompleted;
    };
    VSG_type_name(vsg::Viewer);

    /// update Viewer data structures to match the needs of newly compiled subgraphs
    extern VSG_DECLSPEC void updateViewer(Viewer& viewer, const CompileResult& compileResult);

} // namespace vsg
