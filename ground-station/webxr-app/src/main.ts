/*
 * WebXR Teleop Main Entry
 * Meta Quest 2/3/Pro - STANAG Video + HUD + Controller Input
 */

import * as THREE from 'three';
import { XRSessionManager } from './xr/XRSessionManager';
import { ControllerInput } from './xr/ControllerInput';
import { TeleopCommand } from './xr/TeleopCommand';
import { WebRTCReceiver } from './video/WebRTCReceiver';
import { WebCodecsDecoder } from './video/WebCodecsDecoder';
import { StereoRenderer } from './video/StereoRenderer';
import { KLVParser } from './hud/KLVParser';
import { HUDOverlay } from './hud/HUDOverlay';
import { DataChannel } from './comms/DataChannel';

// Global state
let xrManager: XRSessionManager;
let controllerInput: ControllerInput;
let teleopCommand: TeleopCommand;
let webrtcReceiver: WebRTCReceiver;
let decoder: WebCodecsDecoder;
let stereoRenderer: StereoRenderer;
let klvParser: KLVParser;
let hudOverlay: HUDOverlay;
let dataChannel: DataChannel;

let videoElement: HTMLVideoElement;
let canvas: HTMLCanvasElement;
let gl: WebGL2RenderingContext;
let threeRenderer: THREE.WebGLRenderer;
let scene: THREE.Scene;
let camera: THREE.PerspectiveCamera;
let videoTexture: THREE.VideoTexture;

async function init() {
    console.log('[WebXR Teleop] Initializing...');

    // Get DOM elements
    videoElement = document.getElementById('video') as HTMLVideoElement;
    canvas = document.getElementById('canvas') as HTMLCanvasElement;

    // Initialize Three.js
    threeRenderer = new THREE.WebGLRenderer({
        canvas,
        antialias: true,
        alpha: true,
        preserveDrawingBuffer: true
    });
    threeRenderer.setSize(window.innerWidth, window.innerHeight);
    threeRenderer.xr.enabled = true;
    threeRenderer.xr.setReferenceSpaceType('local-floor');

    scene = new THREE.Scene();
    camera = new THREE.PerspectiveCamera(70, window.innerWidth / window.innerHeight, 0.1, 1000);

    // Video texture for stereo rendering
    videoTexture = new THREE.VideoTexture(videoElement);
    videoTexture.colorSpace = THREE.SRGBColorSpace;
    videoTexture.minFilter = THREE.LinearFilter;
    videoTexture.magFilter = THREE.LinearFilter;

    // Stereo renderer (splits single stream to left/right eyes)
    stereoRenderer = new StereoRenderer(threeRenderer, scene, camera, videoTexture);

    // HUD overlay
    hudOverlay = new HUDOverlay(scene, camera);

    // KLV parser for metadata
    klvParser = new KLVParser();
    klvParser.onMetadata = (metadata) => {
        hudOverlay.updateTelemetry(metadata);
    };

    // WebRTC receiver for video stream
    webrtcReceiver = new WebRTCReceiver({
        signalingUrl: 'wss://your-signaling-server.com', // TODO: configure
        streamPath: 'stanag',
        onTrack: (track) => {
            if (track.kind === 'video') {
                videoElement.srcObject = new MediaStream([track]);
                videoElement.play();
            }
        },
        onDataChannel: (dc) => {
            dataChannel = new DataChannel(dc);
            dataChannel.onCommand = (cmd) => {
                console.log('[Teleop] Received command:', cmd);
            };
        }
    });

    // WebCodecs hardware decoder
    decoder = new WebCodecsDecoder({
        onFrame: (frame) => {
            videoElement.srcObject = new MediaStream([frame]);
        },
        onKLV: (klvData) => {
            klvParser.parse(klvData);
        }
    });

    // XR Session Manager
    xrManager = new XRSessionManager(threeRenderer);
    xrManager.onSessionStart = async () => {
        await webrtcReceiver.connect();
        await dataChannel?.connect();
    };
    xrManager.onSessionEnd = () => {
        webrtcReceiver.disconnect();
        dataChannel?.disconnect();
    };

    // Controller input
    controllerInput = new ControllerInput(xrManager);
    controllerInput.onInput = (input) => {
        teleopCommand.update(input);
    };

    // Teleop command generator
    teleopCommand = new TeleopCommand(dataChannel);
    teleopCommand.onCommand = (cmd) => {
        dataChannel?.send(cmd);
    };

    // Handle window resize
    window.addEventListener('resize', onResize);

    // Start XR button
    const enterXRBtn = document.getElementById('enter-xr') as HTMLButtonElement;
    enterXRBtn.addEventListener('click', () => xrManager.enterXR());

    console.log('[WebXR Teleop] Ready. Click "Enter XR" to start.');
}

function onResize() {
    threeRenderer.setSize(window.innerWidth, window.innerHeight);
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    stereoRenderer.resize();
}

// Animation loop
function animate() {
    threeRenderer.setAnimationLoop(render);
}

function render(timestamp: number, frame: XRFrame) {
    // Update controller input
    controllerInput.update(frame);

    // Render stereo (left/right eyes)
    stereoRenderer.render(frame);

    // Render HUD overlay
    hudOverlay.render(frame);

    // Send teleop command at 50Hz
    if (timestamp % 20 === 0) {
        teleopCommand.sendIfChanged();
    }
}

// Start
init().then(() => animate()).catch(console.error);