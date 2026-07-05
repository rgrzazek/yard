package dev.truckcode.yard.auth;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.Id;
import jakarta.persistence.Table;

// One row per household with an active invite link; regenerating overwrites the token.
@Entity
@Table(name = "group_invite")
public class GroupInvite {

    @Id
    @Column(name = "group_id")
    private Long groupId;

    @Column(nullable = false, unique = true, length = 10)
    private String token;

    public Long getGroupId() { return groupId; }
    public void setGroupId(Long groupId) { this.groupId = groupId; }

    public String getToken() { return token; }
    public void setToken(String token) { this.token = token; }
}
