import { defineStore } from "pinia";
import type { ReportEventPayload } from "@/types/report";

export const useReportChannelStore = defineStore("reportChannel", {
    state: () => ({
        reportChannel: null as any,
    }),

    getters: {},

    actions: {
        /**
         * Initialize the report channel
         * Call this when the web channel is ready
         */
        initializeReportChannel(reportChannel: any) {
            if (!reportChannel) {
                console.warn("Report channel is not available");
                return;
            }

            this.reportChannel = reportChannel;
            console.log("ReportChannel initialized");
        },

        /**
         * Write one or multiple report events
         * @param payload Single event payload or array of event payloads
         */
        writeReportEvent(payload: ReportEventPayload | ReportEventPayload[]) {
            if (!this.reportChannel) {
                console.error("ReportChannel not initialized");
                return;
            }

            try {
                // Convert single event to array for unified processing
                const events = Array.isArray(payload) ? payload : [payload];

                events.forEach(event => {
                    const jsonData = JSON.stringify(event);
                    this.reportChannel.writeReportEvent(jsonData);
                    console.debug("Report event sent:", event.type);
                });
            } catch (error) {
                console.error("Failed to send report event:", error);
            }
        },
    },
});
