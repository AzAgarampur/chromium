// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.task_manager;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

/**
 * This class acts as a bridge between Java and the C++ task manager, providing the methods for
 * receiving updates and fetching specific task information (e.g., memory usage) from the C++ task
 * manager logic.
 */
@JNINamespace("task_manager")
public class TaskManagerServiceBridge {
    /**
     * Adds the observer.
     *
     * @param refreshTimeMillis Specifies how often the observer should be notified. See
     *     TaskManagerObserver in task_manager_observer.h for specifics.
     * @param refreshType Specifies the types of resources that the observer should receive. See
     *     RefreshType in task_manager_observer.h for details.
     * @return The handle of the observer, which should be used on calling removeObserver.
     */
    public ObserverHandle addObserver(
            TaskManagerObserver observer, int refreshTimeMillis, @RefreshType int refreshType) {
        long pointer =
                TaskManagerServiceBridgeJni.get()
                        .addObserver(observer, refreshTimeMillis, refreshType);
        return new ObserverHandle(pointer);
    }

    /** Removes the observer. */
    public void removeObserver(ObserverHandle handle) {
        TaskManagerServiceBridgeJni.get().removeObserver(handle.getPointer());
    }

    // Following methods for fetching task information mirror the functions in
    // task_manager_interface.h. See the file for details.

    public String getTitle(long taskId) {
        return TaskManagerServiceBridgeJni.get().getTitle(taskId);
    }

    public long getMemoryFootprintUsage(long taskId) {
        return TaskManagerServiceBridgeJni.get().getMemoryFootprintUsage(taskId);
    }

    public double getPlatformIndependentCpuUsage(long taskId) {
        return TaskManagerServiceBridgeJni.get().getPlatformIndependentCpuUsage(taskId);
    }

    public long getNetworkUsage(long taskId) {
        return TaskManagerServiceBridgeJni.get().getNetworkUsage(taskId);
    }

    public long getProcessId(long taskId) {
        return TaskManagerServiceBridgeJni.get().getProcessId(taskId);
    }

    public boolean isTaskKillable(long taskId) {
        return TaskManagerServiceBridgeJni.get().isTaskKillable(taskId);
    }

    public void killTask(long taskId) {
        TaskManagerServiceBridgeJni.get().killTask(taskId);
    }

    public static class ObserverHandle {
        private long mPointer;

        ObserverHandle(long pointer) {
            mPointer = pointer;
        }

        long getPointer() {
            return mPointer;
        }
    }

    @NativeMethods
    public interface Natives {
        long addObserver(
                TaskManagerObserver observer, int refreshTimeMillis, @RefreshType int refreshType);

        void removeObserver(long pointer);

        String getTitle(long taskid);

        long getMemoryFootprintUsage(long taskId);

        double getPlatformIndependentCpuUsage(long taskId);

        long getNetworkUsage(long taskId);

        long getProcessId(long taskId);

        boolean isTaskKillable(long taskId);

        void killTask(long taskId);
    }
}
