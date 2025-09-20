import { type Component, For, Index, Show, createSignal } from "solid-js";

import Button from "./components/button";
import { Layout } from "./components/layout/layout";
import { Tooltip } from "./components/tooltip";
import { useStore } from "./contexts/store";
import { useToast } from "./contexts/toast";

const API_URL = import.meta.env.PROD
  ? `http://${window.location.host}/`
  : import.meta.env.VITE_BASE_URL;

export const ResetScheduleButton = () => {
  const { toast } = useToast();

  return (
    <button
      type="button"
      onClick={async () => {
        try {
          const response = await fetch(`${API_URL}api/schedule/clear`);

          if (response.ok) {
            toast("Reset schedule successfully", 2000);
          }
        } catch {
          toast("Failed to reset schedule", 2000);
        }
      }}
      class="w-full bg-blue-600 hover:bg-red-600 text-white border-0 px-4 py-3 uppercase text-sm leading-6 tracking-wider cursor-pointer font-bold hover:opacity-80 active:translate-y-[-1px] transition-all rounded"
    >
      Reset Scheduler
    </button>
  );
};

export const ToggleScheduleButton = () => {
  const [store] = useStore();
  const { toast } = useToast();

  return (
    <button
      type="button"
      onClick={async () => {
        if (store.isActiveScheduler) {
          try {
            const response = await fetch(`${API_URL}api/schedule/stop`);
            if (response.ok) toast("Stopped schedule successfully", 2000);
          } catch {
            toast("Failed to stop schedule", 2000);
          }
        } else {
          try {
            // Push day/night lists if present
            if (store.scheduleDay?.length > 0) {
              const r1 = await fetch(`${API_URL}api/schedule/day`, {
                method: "POST",
                headers: { "Content-Type": "application/x-www-form-urlencoded" },
                body: `schedule=${encodeURIComponent(JSON.stringify(store.scheduleDay))}`,
              });
              if (!r1.ok) throw new Error("Failed to set day schedule");
            }
            if (store.scheduleNight?.length > 0) {
              const r2 = await fetch(`${API_URL}api/schedule/night`, {
                method: "POST",
                headers: { "Content-Type": "application/x-www-form-urlencoded" },
                body: `schedule=${encodeURIComponent(JSON.stringify(store.scheduleNight))}`,
              });
              if (!r2.ok) throw new Error("Failed to set night schedule");
            }
            const r3 = await fetch(`${API_URL}api/schedule/start`);
            if (!r3.ok) throw new Error("Failed to start scheduler");
            toast("Scheduler started", 2000);
          } catch (error) {
            console.error("Failed to start scheduler:", error);
            toast("Failed to start schedule", 2000);
          }
        }
      }}
      class="w-full bg-blue-600 text-white border-0 px-4 py-3 uppercase text-sm leading-6 tracking-wider cursor-pointer font-bold hover:opacity-80 active:translate-y-[-1px] transition-all rounded"
    >
      {store.isActiveScheduler ? "Stop" : "Start"} Scheduler
    </button>
  );
};

const Scheduler: Component = () => {
  const [store, actions] = useStore();
  const [period, setPeriod] = createSignal<'day' | 'night'>('day');
  const currentItems = () => (period() === 'day' ? store.scheduleDay : store.scheduleNight);
  const setCurrentItems = (items: any[]) =>
    period() === 'day' ? actions.setScheduleDay(items) : actions.setScheduleNight(items);


  const handleAddItem = () => {
    const base = currentItems();
    const next = [...base, { pluginId: store.plugins[0]?.id || 1, duration: 1 }];
    setCurrentItems(next);
  };

  const handleRemoveItem = (index: number) => {
    const base = currentItems();
    const next = base.filter((_, i) => i !== index);
    setCurrentItems(next);
  };

  const handlePluginChange = (index: number, pluginId: number) => {
    const base = currentItems();
    const next = base.map((item, i) => (i === index ? { ...item, pluginId } : item));
    setCurrentItems(next);
  };

  const handleDurationChange = (index: number, duration: number) => {
    const base = currentItems();
    const next = base.map((item, i) => (i === index ? { ...item, duration } : item));
    setCurrentItems(next);
  };

  return (
    <Layout
      content={
        <div class="space-y-3 p-5">
          <h3 class="text-4xl text-white  tracking-wide">Scheduler</h3>


          <div class="flex items-center justify-between mb-4">
            <div class="inline-flex rounded-md shadow-sm" role="group">
              <button type="button" class={`px-3 py-2 text-sm font-medium border ${period()==='day' ? 'bg-blue-600 text-white border-blue-700' : 'bg-white text-gray-700 border-gray-200'}`} onClick={() => setPeriod('day')}>Edit Day</button>
              <button type="button" class={`px-3 py-2 text-sm font-medium border -ml-px ${period()==='night' ? 'bg-blue-600 text-white border-blue-700' : 'bg-white text-gray-700 border-gray-200'}`} onClick={() => setPeriod('night')}>Edit Night</button>
            </div>
            <div class="text-sm text-gray-600">Current period: <span class="font-semibold">{store.currentPeriod}</span></div>
          </div>

          <div class="bg-white p-6 rounded-md">
            <div class="space-y-2">
              <Show
                when={currentItems()?.length > 0}
                fallback={<div class="text-md text-gray-500 italic">No schedule set</div>}
              >
                <Index each={currentItems()}>
                  {(item, index) => (
                    <div
                      class={`flex items-center gap-4 p-4 rounded-lg shadow-sm border hover:border-gray-200 transition-all duration-200 ${
                        store.plugin === item().pluginId
                          ? "bg-green-300 border-green-500"
                          : "bg-white border-gray-100"
                      }`}
                    >
                      <Show
                        when={store.isActiveScheduler}
                        fallback={
                          <>
                            <div class="flex-1">
                              <select
                                value={item().pluginId}
                                onChange={(e) =>
                                  handlePluginChange(index, parseInt(e.currentTarget.value))
                                }
                                class="w-full px-3 py-2 bg-gray-50 border border-gray-200 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-blue-500 outline-none transition-all duration-200 disabled:border-0"
                              >
                                <For each={store.plugins}>
                                  {(plugin) => <option value={plugin.id}>{plugin.name}</option>}
                                </For>
                              </select>
                            </div>
                            <div class="flex items-center gap-3">
                              <div class="relative">
                                <input
                                  type="number"
                                  min="1"
                                  value={item().duration}
                                  onInput={(e) =>
                                    handleDurationChange(index, parseInt(e.currentTarget.value))
                                  }
                                  class="pr-16 pl-3 py-2 w-32 bg-gray-50 border border-gray-200 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-blue-500 outline-none transition-all duration-200 disabled:border-0"
                                />
                                <span class="absolute right-3 top-1/2 -translate-y-1/2 text-gray-400 text-sm">
                                  {item().duration > 1 ? "seconds" : "second"}
                                </span>
                              </div>
                            </div>

                            <button
                              type="button"
                              onClick={() => handleRemoveItem(index)}
                              class="p-2 text-gray-500 hover:text-red-500 hover:bg-red-50 rounded-lg transition-all duration-200"
                              aria-label="Remove item"
                            >
                              <i class="fas fa-trash-alt text-lg" />
                            </button>
                          </>
                        }
                      >
                        <div class="w-full">
                          {store.plugins?.find((p) => p.id === item().pluginId)?.name ??
                            "Unknown Plugin"}
                        </div>
                        <div class="flex items-center gap-3">
                          <div class="relative">
                            {item().duration}
                            <span class="ml-2 text-sm text-gray-500">
                              {item().duration > 1 ? "seconds" : "second"}
                            </span>
                          </div>
                        </div>
                      </Show>
                    </div>
                  )}
                </Index>
              </Show>
            </div>
          </div>
        </div>
      }
      sidebar={
        <>
          <div class="space-y-6">
            <div class="space-y-3">
              <h3 class="text-sm font-semibold text-gray-700 uppercase tracking-wide">Controls</h3>

              <div class="flex flex-col gap-2">
                <div class="p-3 border rounded-md">
                  <div class="text-xs font-semibold text-gray-600 mb-2 uppercase tracking-wide">Bounds</div>
                  <div class="flex flex-col gap-2">
                    <label class="text-sm text-gray-700">Day start
                      <input type="time" value={store.dayStart} onInput={(e) => actions.setDayStart((e.currentTarget as HTMLInputElement).value)} class="ml-2 border px-2 py-1 rounded" />
                    </label>
                    <label class="text-sm text-gray-700">Night start
                      <input type="time" value={store.nightStart} onInput={(e) => actions.setNightStart((e.currentTarget as HTMLInputElement).value)} class="ml-2 border px-2 py-1 rounded" />
                    </label>
                    <Button
                      onClick={async () => {
                        try {
                          const resp = await fetch(`${API_URL}api/schedule/bounds`, {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                            body: `dayStart=${encodeURIComponent(store.dayStart)}&nightStart=${encodeURIComponent(store.nightStart)}`,
                          });
                          if (!resp.ok) throw new Error('failed');
                          useToast().toast('Bounds saved', 1500);
                        } catch {
                          useToast().toast('Failed to save bounds', 2000);
                        }
                      }}
                      class="mt-2"
                    >Save Bounds</Button>
                  </div>
                </div>

                <Button onClick={handleAddItem} class="hover:bg-green-600 transition-colors">
                  <i class="fa-solid fa-plus mr-2" />
                  Add Plugin
                </Button>

                <Show when={(store.scheduleDay.length + store.scheduleNight.length) > 0}>
                  <div class="my-6 border-t border-gray-200" />
                  <ToggleScheduleButton />
                </Show>

                <Show when={store.isActiveScheduler}>
                  <div class="my-6 border-t border-gray-200" />
                  <Tooltip text="Removes all items and resets state">
                    <ResetScheduleButton />
                  </Tooltip>
                </Show>
              </div>
            </div>
          </div>
          <div class="mt-auto pt-6 border-t border-gray-200">
            <Tooltip text="Return to main editor">
              <a
                href="#/"
                class="inline-flex items-center text-gray-700 hover:text-gray-900 font-medium"
              >
                <i class="fa-solid fa-arrow-left mr-2" />
                Back
              </a>
            </Tooltip>
          </div>
        </>
      }
    />
  );
};

export default Scheduler;
