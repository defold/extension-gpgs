<ul>
{% for field in include.fields %}
<li><strong>{{ field.name }}</strong> <code>{{ field.type }}</code> - {{ field.desc | markdownify }}</li>
{% endfor %}
</ul>
