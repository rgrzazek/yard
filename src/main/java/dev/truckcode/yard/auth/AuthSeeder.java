package dev.truckcode.yard.auth;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.CommandLineRunner;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Component;

// No self-service registration exists, so accounts are seeded from properties.
// Real values live only in the gitignored application-local.properties; a
// blank username/password pair is skipped, so this is a no-op until set.
@Component
public class AuthSeeder implements CommandLineRunner {

    private final UserRepository userRepository;
    private final PasswordEncoder passwordEncoder;

    @Value("${app.seed.user1.username:}")
    private String user1Username;

    @Value("${app.seed.user1.password:}")
    private String user1Password;

    @Value("${app.seed.user2.username:}")
    private String user2Username;

    @Value("${app.seed.user2.password:}")
    private String user2Password;

    public AuthSeeder(UserRepository userRepository, PasswordEncoder passwordEncoder) {
        this.userRepository = userRepository;
        this.passwordEncoder = passwordEncoder;
    }

    @Override
    public void run(String... args) {
        // Reuse whichever seed account already exists so both land in the same
        // household; only draw a fresh group id if neither has been created yet.
        Long groupId = userRepository.findByUsername(user1Username)
                .or(() -> userRepository.findByUsername(user2Username))
                .map(User::getGroupId)
                .orElseGet(userRepository::nextGroupId);

        seedUser(user1Username, user1Password, groupId);
        seedUser(user2Username, user2Password, groupId);
    }

    private void seedUser(String username, String password, Long groupId) {
        if (username == null || username.isBlank() || password == null || password.isBlank()) return;
        if (userRepository.findByUsername(username).isPresent()) return;

        var user = new User();
        user.setUsername(username);
        user.setPasswordHash(passwordEncoder.encode(password));
        user.setGroupId(groupId);
        userRepository.save(user);
    }
}
