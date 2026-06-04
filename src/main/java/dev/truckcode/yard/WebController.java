package dev.truckcode.yard;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.GetMapping;

@Controller
public class WebController {

  @GetMapping("/")
  public String home() {
    return "home";
  }

  @GetMapping("/about_me")
  public String aboutMe() {
    return "about_me";
  }

  @GetMapping("/sim")
  public String sim() {
    return "simulator";
  }

  @GetMapping("/shunt")
  public String shunt() {
    return "shunt";
  }

}
