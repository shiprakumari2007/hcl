package com.project.safetywearable.dto;

public class DashboardSummaryDto {
    private long totalWorkers;
    private long onlineWorkers;
    private long offlineWorkers;
    private long activeAlerts;
    private long normalWorkers;
    private long warningWorkers;
    private String siteName;

    public DashboardSummaryDto() {}

    public DashboardSummaryDto(long totalWorkers, long onlineWorkers, long offlineWorkers,
                               long activeAlerts, long normalWorkers, long warningWorkers, String siteName) {
        this.totalWorkers = totalWorkers;
        this.onlineWorkers = onlineWorkers;
        this.offlineWorkers = offlineWorkers;
        this.activeAlerts = activeAlerts;
        this.normalWorkers = normalWorkers;
        this.warningWorkers = warningWorkers;
        this.siteName = siteName;
    }

    public long getTotalWorkers() { return totalWorkers; }
    public void setTotalWorkers(long totalWorkers) { this.totalWorkers = totalWorkers; }

    public long getOnlineWorkers() { return onlineWorkers; }
    public void setOnlineWorkers(long onlineWorkers) { this.onlineWorkers = onlineWorkers; }

    public long getOfflineWorkers() { return offlineWorkers; }
    public void setOfflineWorkers(long offlineWorkers) { this.offlineWorkers = offlineWorkers; }

    public long getActiveAlerts() { return activeAlerts; }
    public void setActiveAlerts(long activeAlerts) { this.activeAlerts = activeAlerts; }

    public long getNormalWorkers() { return normalWorkers; }
    public void setNormalWorkers(long normalWorkers) { this.normalWorkers = normalWorkers; }

    public long getWarningWorkers() { return warningWorkers; }
    public void setWarningWorkers(long warningWorkers) { this.warningWorkers = warningWorkers; }

    public String getSiteName() { return siteName; }
    public void setSiteName(String siteName) { this.siteName = siteName; }
}
