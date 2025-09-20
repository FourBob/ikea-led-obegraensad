export enum SYSTEM_STATUS {
  NONE = "draw",
  WSBINARY = "wsbinary",
  // SYSTEM
  UPDATE = "update",
  LOADING = "loading",
}

export interface ScheduleItem {
  pluginId: number;
  duration: number;
}

export type Period = "day" | "night";

export interface StoreActions {
  setIsActiveScheduler: (isActive: boolean) => void;
  setRotation: (rotation: number) => void;
  setPlugins: (plugins: []) => void;
  setPlugin: (plugin: number) => void;
  setBrightness: (brightness: number) => void;
  setIndexMatrix: (indexMatrix: number[]) => void;
  setLeds: (leds: number[]) => void;
  setSystemStatus: (systemStatus: SYSTEM_STATUS) => void;
  setSchedule: (items: ScheduleItem[]) => void; // active schedule (compat)
  setScheduleDay: (items: ScheduleItem[]) => void;
  setScheduleNight: (items: ScheduleItem[]) => void;
  setDayStart: (s: string) => void;
  setNightStart: (s: string) => void;
  setCurrentPeriod: (p: Period) => void;
  setArtnetUniverse: (artnetUniverse: number) => void;
  setBuildTime: (s: string) => void;
  setVersion: (s: string) => void;
  send: (message: string | ArrayBuffer) => void;
}

export interface Store {
  isActiveScheduler: boolean;
  rotation: number;
  brightness: number;
  indexMatrix: number[];
  leds: number[];
  plugins: { id: number; name: string }[];
  plugin: number;
  artnetUniverse: number;
  systemStatus: SYSTEM_STATUS;
  connectionState: () => number;
  connectionStatus?: string;
  schedule: ScheduleItem[]; // active schedule (compat for UI)
  scheduleDay: ScheduleItem[];
  scheduleNight: ScheduleItem[];
  dayStart: string; // HH:MM
  nightStart: string; // HH:MM
  currentPeriod: Period;
  buildTime?: string;
  version?: string;
}

export interface IToastContext {
  toast: (text: string, duration: number) => void;
}
