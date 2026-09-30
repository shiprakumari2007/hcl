package com.project.safetywearable.dto;

public class AcknowledgeAlertRequestDto {
    private String supervisor;
    private String note;

    public AcknowledgeAlertRequestDto() {}

    public String getSupervisor() { return supervisor; }
    public void setSupervisor(String supervisor) { this.supervisor = supervisor; }

    public String getNote() { return note; }
    public void setNote(String note) { this.note = note; }
}
