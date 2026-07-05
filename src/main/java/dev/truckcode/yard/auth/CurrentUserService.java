package dev.truckcode.yard.auth;

import org.springframework.security.core.Authentication;
import org.springframework.stereotype.Service;

import java.util.Optional;

// Single place that turns a Spring Security Authentication into the domain
// User/groupId — every group-scoped query should get its group id from here,
// never from a request parameter.
@Service
public class CurrentUserService {

    private final UserRepository userRepository;

    public CurrentUserService(UserRepository userRepository) {
        this.userRepository = userRepository;
    }

    public Optional<User> currentUser(Authentication authentication) {
        if (authentication == null || !authentication.isAuthenticated()) return Optional.empty();
        return userRepository.findByUsername(authentication.getName());
    }

    public Optional<Long> currentGroupId(Authentication authentication) {
        return currentUser(authentication).map(User::getGroupId);
    }

    public Optional<Long> currentUserId(Authentication authentication) {
        return currentUser(authentication).map(User::getId);
    }
}
