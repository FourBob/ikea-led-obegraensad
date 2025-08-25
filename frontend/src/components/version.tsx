import { type Component } from "solid-js";

const buildTime = (import.meta as any).env?.VITE_BUILD_TIME as string | undefined;
const version = (import.meta as any).env?.VITE_APP_VERSION as string | undefined;

export const VersionBadge: Component = () => {
  const label = version ? `v${version}` : buildTime ? new Date(buildTime).toLocaleString() : "";
  if (!label) return null as any;
  return (
    <div class="fixed right-2 bottom-2 text-[10px] text-gray-500 opacity-70 select-none">
      {label}
    </div>
  );
};

export default VersionBadge;

