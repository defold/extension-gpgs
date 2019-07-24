<table>
    <thead>
        <tr>
            <th>Parameter</th>
            <th>Type</th>
            <th>Description</th>
        </tr>
    </thead>
    <tbody>
    {% for param in include.params %}
        <tr>
            <td><strong>{{ param.name }}</strong></td>
            <td><code>{{ param.type }}</code></td>
            <td>{{ param.desc | markdownify }}

                {% if param.type == "table" %}
                {% include type-table.md fields=param.members %}
                {% endif %}
            </td>
        </tr>
        {% endfor %}
    </tbody>
</table>
