import { defineConfig, loadEnv } from 'vite';
import solidPlugin from 'vite-plugin-solid';
import tailwindcss from '@tailwindcss/vite';
import { viteSingleFile } from 'vite-plugin-singlefile';

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '');
  const buildTime = new Date().toISOString();
  return {
    plugins: [tailwindcss(), solidPlugin(), viteSingleFile()],
    server: { port: 3000 },
    build: { target: 'esnext' },
    define: {
      'import.meta.env.VITE_BUILD_TIME': JSON.stringify(env.VITE_BUILD_TIME || buildTime),
      'import.meta.env.VITE_APP_VERSION': JSON.stringify(env.VITE_APP_VERSION || ''),
    },
  };
});
