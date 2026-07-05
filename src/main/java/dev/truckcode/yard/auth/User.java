package dev.truckcode.yard.auth;

import jakarta.persistence.*;

// Table name avoids the "user" SQL keyword.
@Entity
@Table(name = "app_user")
public class User {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false, unique = true)
    private String username;

    // Nullable so an account can exist without a local password (e.g. an OAuth-only login added later).
    @Column(name = "password_hash")
    private String passwordHash;

    // Null until the user starts or joins a household — see GroupService.
    // Plain shared number, not a foreign key — see Recipe.groupId for why there's no backing table.
    @Column(name = "group_id")
    private Long groupId;

    public Long getId() { return id; }
    public void setId(Long id) { this.id = id; }

    public String getUsername() { return username; }
    public void setUsername(String username) { this.username = username; }

    public String getPasswordHash() { return passwordHash; }
    public void setPasswordHash(String passwordHash) { this.passwordHash = passwordHash; }

    public Long getGroupId() { return groupId; }
    public void setGroupId(Long groupId) { this.groupId = groupId; }
}
