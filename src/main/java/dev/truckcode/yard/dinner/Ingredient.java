package dev.truckcode.yard.dinner;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import jakarta.persistence.Embeddable;
import jakarta.validation.constraints.Size;

@Embeddable
public class Ingredient {

    @Size(max = 100)
    private String name;
    @Size(max = 100)
    private String quantity;
    @Size(max = 100)
    private String display;
    private String type;

    public Ingredient() {}

    @JsonCreator
    public Ingredient(
            @JsonProperty("name") String name,
            @JsonProperty("quantity") String quantity,
            @JsonProperty("display") String display,
            @JsonProperty("type") String type
    ) {
        this.name = name;
        this.quantity = quantity;
        this.display = display;
        this.type = type;
    }

    public String name()     { return name; }
    public String quantity() { return quantity; }
    public String display()  { return display; }
    public String type()     { return type; }

    public void setName(String name)         { this.name = name; }
    public void setQuantity(String quantity) { this.quantity = quantity; }
    public void setDisplay(String display)   { this.display = display; }
    public void setType(String type)         { this.type = type; }

    public IngredientType getIngredientType() {
        if (type == null) return null;
        try { return IngredientType.valueOf(type.toUpperCase()); }
        catch (IllegalArgumentException e) { return null; }
    }
}
