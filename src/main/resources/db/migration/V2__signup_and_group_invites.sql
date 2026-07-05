-- Self-service signup: a fresh account has no household until the user
-- starts one or joins one via an invite link.
ALTER TABLE app_user ALTER COLUMN group_id DROP NOT NULL;

-- One active invite link per household; regenerating replaces the token.
CREATE TABLE group_invite (
    group_id BIGINT PRIMARY KEY,
    token    VARCHAR(10) NOT NULL UNIQUE
);
