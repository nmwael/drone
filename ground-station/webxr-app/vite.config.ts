import { defineConfig } from 'vite';

export default defineConfig({
    root: '.',
    publicDir: 'public',
    build: {
        outDir: 'dist',
        sourcemap: true,
        target: 'esnext',
        modulePreload: {
            polyfill: true
        }
    },
    server: {
        port: 3000,
        host: true,
        https: {
            // Generate self-signed cert for WebXR (requires HTTPS)
            // Run: openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes
            key: '../certs/key.pem',
            cert: '../certs/cert.pem'
        },
        headers: {
            // Required for WebXR
            'Permissions-Policy': 'xr-spatial-tracking=*, camera=*, microphone=*',
            'Cross-Origin-Embedder-Policy': 'require-corp',
            'Cross-Origin-Opener-Policy': 'same-origin'
        }
    },
    optimizeDeps: {
        include: ['three']
    },
    define: {
        'process.env.NODE_ENV': JSON.stringify(process.env.NODE_ENV || 'development')
    }
});