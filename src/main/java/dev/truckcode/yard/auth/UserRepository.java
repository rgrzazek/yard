package dev.truckcode.yard.auth;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;

import java.util.Optional;

public interface UserRepository extends JpaRepository<User, Long> {
    Optional<User> findByUsername(String username);

    long countByGroupId(Long groupId);

    // Draws a fresh household id from group_id_seq — see Recipe.groupId for why
    // there's no group table to generate an id from instead.
    @Query(value = "SELECT nextval('group_id_seq')", nativeQuery = true)
    Long nextGroupId();
}
