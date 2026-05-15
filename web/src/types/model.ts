export type ModelNetwork = "online" | "local" | "private";

export const DEFAULT_MODEL_NETWORK: ModelNetwork = "online";

export const MODEL_NETWORK_ICON_MAP: Record<ModelNetwork, string> = {
    online: "icon_model_online",
    local: "icon_model_local",
    private: "icon_model_online",
};

export function isModelNetwork(value: unknown): value is ModelNetwork {
    return value === "online" || value === "local" || value === "private";
}

export enum ModelAbility {
    MaUnknown = 0,
    MaText = 1,
    MaImage = 1 << 1,
    MaFunctionCalling = 1 << 2,
    MaReasoning = 1 << 3
};

export interface Model {
    id: string;
    name: string;
    icon: string;
    network: ModelNetwork;
    provider: string;
    ability: ModelAbility;
}
