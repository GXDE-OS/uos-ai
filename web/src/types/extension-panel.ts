import type { InjectionKey, VNode } from 'vue';

/**
 * Extension Panel API exposed via provide/inject
 * Allows any descendant component to open/close/toggle the extension panel
 */

type ComponentContent = () => VNode;

/** Configuration options for opening extension panel */
export interface OpenExtensionPanelOptions {
  /** Component or vnode to render in panel */
  content: ComponentContent;
  /** Automatically enter fullscreen mode when opening */
  autoFullscreen?: boolean;
}

export interface ExtensionPanelAPI {
  /** Open extension panel with specified content */
  openExtensionPanel: (content: ComponentContent) => void;
  /** Open extension panel with options (for fullscreen mode, etc.) */
  openExtensionPanelWithOptions: (options: OpenExtensionPanelOptions) => void;
  /** Close extension panel */
  closeExtensionPanel: () => void;
  /** Toggle extension panel visibility */
  toggleExtensionPanel: () => void;
  /** Control chat area visibility */
  setChatAreaVisible: (visible: boolean) => void;
  /** Expand panel to fill the entire workspace (chat area animates out) */
  setPanelFullscreen: (value: boolean) => void;
}

/** Injection key for ExtensionPanelAPI */
export const EXTENSION_PANEL_KEY: InjectionKey<ExtensionPanelAPI> = Symbol('extensionPanel');

/** Extension panel state structure */
export interface ExtensionPanelState {
  /** Chat area visibility */
  showChatArea: boolean;
  /** Extension panel visibility */
  showExtensionPanel: boolean;
  /** Current extension panel content (component or vnode) */
  extensionContent: ComponentContent | null;
  /** Panel fullscreen: chat area animates out, panel fills the entire workspace */
  panelFullscreen: boolean;
  /** Chat area auto-hidden by window resize */
  isChatAreaAutoHiddenByResize: boolean;
  /** Fullscreen auto-enabled by window resize */
  isPanelFullscreenAutoByResize: boolean;
}
